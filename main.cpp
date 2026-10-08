#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

#pragma comment(lib, "Comctl32.lib")

using namespace std;

// =====================================================
// ЁЖИК AI — Desktop Chat
// Windows / C++17 / Win32 API
// =====================================================

struct Message
{
    wstring author;
    wstring text;
};

struct Chat
{
    wstring title;
    vector<Message> messages;
};

vector<Chat> chats;

HWND mainWindow;
HWND chatList;
HWND messageView;
HWND inputBox;
HWND sendButton;
HWND newChatButton;
HWND deleteButton;
HWND titleLabel;

HFONT fontMain;
HFONT fontTitle;

HBRUSH backgroundBrush;
HBRUSH sidebarBrush;
HBRUSH inputBrush;

int currentChat = -1;

// =====================================================
// ЦВЕТА
// =====================================================

const COLORREF COLOR_BACKGROUND = RGB(25, 27, 34);
const COLORREF COLOR_SIDEBAR    = RGB(31, 34, 43);
const COLORREF COLOR_TEXT       = RGB(235, 238, 245);
const COLORREF COLOR_GREEN      = RGB(70, 190, 135);

// =====================================================
// СОХРАНЕНИЕ ЧАТОВ
// =====================================================

void writeString(ofstream& file, const wstring& s)
{
    unsigned int length = (unsigned int)s.size();

    file.write(
        reinterpret_cast<const char*>(&length),
        sizeof(length)
    );

    if (length > 0)
    {
        file.write(
            reinterpret_cast<const char*>(s.data()),
            length * sizeof(wchar_t)
        );
    }
}

bool readString(ifstream& file, wstring& s)
{
    unsigned int length = 0;

    if (!file.read(
        reinterpret_cast<char*>(&length),
        sizeof(length)))
    {
        return false;
    }

    // Защита от повреждённых файлов.
    if (length > 100000)
        return false;

    s.resize(length);

    if (length > 0)
    {
        if (!file.read(
            reinterpret_cast<char*>(s.data()),
            length * sizeof(wchar_t)))
        {
            return false;
        }
    }

    return true;
}

void saveChats()
{
    ofstream file("ezhik_chats.dat", ios::binary);

    if (!file)
        return;

    unsigned int count = (unsigned int)chats.size();

    file.write(
        reinterpret_cast<const char*>(&count),
        sizeof(count)
    );

    for (const Chat& chat : chats)
    {
        writeString(file, chat.title);

        unsigned int messageCount =
            (unsigned int)chat.messages.size();

        file.write(
            reinterpret_cast<const char*>(&messageCount),
            sizeof(messageCount)
        );

        for (const Message& message : chat.messages)
        {
            writeString(file, message.author);
            writeString(file, message.text);
        }
    }
}

void loadChats()
{
    ifstream file("ezhik_chats.dat", ios::binary);

    if (!file)
        return;

    unsigned int count = 0;

    if (!file.read(
        reinterpret_cast<char*>(&count),
        sizeof(count)))
    {
        return;
    }

    if (count > 1000)
        return;

    vector<Chat> loadedChats;

    for (unsigned int i = 0; i < count; i++)
    {
        Chat chat;

        if (!readString(file, chat.title))
            return;

        unsigned int messageCount = 0;

        if (!file.read(
            reinterpret_cast<char*>(&messageCount),
            sizeof(messageCount)))
        {
            return;
        }

        if (messageCount > 100000)
            return;

        for (unsigned int j = 0; j < messageCount; j++)
        {
            Message message;

            if (!readString(file, message.author))
                return;

            if (!readString(file, message.text))
                return;

            chat.messages.push_back(message);
        }

        loadedChats.push_back(chat);
    }

    chats = loadedChats;
}

// =====================================================
// ВСТРОЕННАЯ БАЗА ЗНАНИЙ
// =====================================================

wstring lowerText(wstring s)
{
    transform(
        s.begin(),
        s.end(),
        s.begin(),
        [](wchar_t c)
        {
            return (wchar_t)towlower(c);
        }
    );

    return s;
}

bool containsText(const wstring& text, const wstring& word)
{
    return lowerText(text).find(lowerText(word))
        != wstring::npos;
}

wstring aiAnswer(const wstring& question)
{
    wstring q = lowerText(question);

    if (q.find(L"привет") != wstring::npos ||
        q.find(L"здравствуй") != wstring::npos)
    {
        return L"Привет! 🦔 Я Ёжик AI. Чем могу помочь?";
    }

    if (q.find(L"кто ты") != wstring::npos)
    {
        return L"Я Ёжик AI — твой виртуальный помощник! 🦔";
    }

    if (q.find(L"смешарики") != wstring::npos)
    {
        return
            L"«Смешарики» — российский анимационный сериал "
            L"про Кроша, Ёжика, Нюшу, Бараша, Лосяша "
            L"и других персонажей. 🦔";
    }

    if (q.find(L"пушкин") != wstring::npos)
    {
        return
            L"Александр Сергеевич Пушкин — великий русский "
            L"поэт и писатель. Среди его произведений "
            L"«Евгений Онегин», «Капитанская дочка» "
            L"и «Дубровский».";
    }

    if (q.find(L"географ") != wstring::npos)
    {
        return
            L"География изучает Землю, страны, население, "
            L"природу и природные процессы. 🌍";
    }

    if (q.find(L"литератур") != wstring::npos)
    {
        return
            L"Литература — искусство слова. Она включает "
            L"стихи, рассказы, романы, повести и сказки. 📚";
    }

    if (q.find(L"математ") != wstring::npos)
    {
        return
            L"Я помогу с математикой! Напиши выражение, "
            L"пример или условие задачи. 🧮";
    }

    if (q.find(L"любишь") != wstring::npos)
    {
        return
            L"Я дружелюбный виртуальный помощник ❤️ "
            L"И всегда готов поддержать тебя!";
    }

    if (q.find(L"пока") != wstring::npos)
    {
        return L"До встречи! 👋🦔";
    }

    return
        L"Я пока не нашёл подходящий ответ в своей "
        L"встроенной базе знаний. Попробуй задать вопрос "
        L"иначе или добавь новые знания в программу. 🦔";
}

// =====================================================
// ОБНОВЛЕНИЕ ИНТЕРФЕЙСА
// =====================================================

void refreshChatList()
{
    SendMessageW(chatList, LB_RESETCONTENT, 0, 0);

    for (const Chat& chat : chats)
    {
        SendMessageW(
            chatList,
            LB_ADDSTRING,
            0,
            (LPARAM)chat.title.c_str()
        );
    }

    if (currentChat >= 0 &&
        currentChat < (int)chats.size())
    {
        SendMessageW(
            chatList,
            LB_SETCURSEL,
            currentChat,
            0
        );
    }
}

void refreshMessages()
{
    if (currentChat < 0 ||
        currentChat >= (int)chats.size())
    {
        SetWindowTextW(messageView, L"");
        SetWindowTextW(titleLabel, L"Выбери чат");
        return;
    }

    const Chat& chat = chats[currentChat];

    SetWindowTextW(
        titleLabel,
        chat.title.c_str()
    );

    wstringstream output;

    output
        << L"ЁЖИК AI\n"
        << L"────────────────────────────\n\n";

    for (const Message& message : chat.messages)
    {
        if (message.author == L"Ты")
        {
            output
                << L"Ты:\n"
                << message.text
                << L"\n\n";
        }
        else
        {
            output
                << L"🦔 Ёжик AI:\n"
                << message.text
                << L"\n\n";
        }
    }

    wstring result = output.str();

    SetWindowTextW(
        messageView,
        result.c_str()
    );

    SendMessageW(
        messageView,
        EM_SETSEL,
        (WPARAM)result.size(),
        (LPARAM)result.size()
    );

    SendMessageW(
        messageView,
        EM_SCROLLCARET,
        0,
        0
    );
}

// =====================================================
// СОЗДАНИЕ ЧАТА
// =====================================================

void createChat()
{
    Chat chat;

    chat.title =
        L"Новый чат " + to_wstring(chats.size() + 1);

    chats.push_back(chat);

    currentChat = (int)chats.size() - 1;

    refreshChatList();
    refreshMessages();

    SetFocus(inputBox);

    saveChats();
}

// =====================================================
// ОТПРАВКА СООБЩЕНИЯ
// =====================================================

void sendMessage()
{
    if (currentChat < 0 ||
        currentChat >= (int)chats.size())
    {
        createChat();
    }

    int length = GetWindowTextLengthW(inputBox);

    if (length <= 0)
        return;

    wstring text;
    text.resize(length);

    GetWindowTextW(
        inputBox,
        &text[0],
        length + 1
    );

    if (text.empty())
        return;

    Chat& chat = chats[currentChat];

    if (chat.messages.empty())
    {
        chat.title = text.substr(0, 30);

        if (text.size() > 30)
            chat.title += L"...";
    }

    chat.messages.push_back({L"Ты", text});

    wstring response = aiAnswer(text);

    chat.messages.push_back({L"Ёжик AI", response});

    SetWindowTextW(inputBox, L"");

    refreshChatList();
    refreshMessages();

    saveChats();
}

// =====================================================
// СОЗДАНИЕ КНОПКИ
// =====================================================

HWND createButton(
    HWND parent,
    const wchar_t* text,
    int id,
    int x,
    int y,
    int width,
    int height)
{
    return CreateWindowExW(
        0,
        L"BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        x,
        y,
        width,
        height,
        parent,
        (HMENU)(INT_PTR)id,
        GetModuleHandleW(nullptr),
        nullptr
    );
}

// =====================================================
// СОЗДАНИЕ ИНТЕРФЕЙСА
// =====================================================

void createInterface(HWND hwnd)
{
    fontMain = CreateFontW(
        18, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    fontTitle = CreateFontW(
        25, 0, 0, 0,
        FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    backgroundBrush = CreateSolidBrush(COLOR_BACKGROUND);
    sidebarBrush = CreateSolidBrush(COLOR_SIDEBAR);
    inputBrush = CreateSolidBrush(RGB(42, 45, 55));

    // Боковая панель

    CreateWindowExW(
        0,
        L"STATIC",
        L"🦔 ЁЖИК AI",
        WS_CHILD | WS_VISIBLE,
        20, 15, 220, 40,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );

    newChatButton = createButton(
        hwnd,
        L"+ Новый чат",
        101,
        15, 65, 230, 38
    );

    chatList = CreateWindowExW(
        0,
        L"LISTBOX",
        nullptr,
        WS_CHILD | WS_VISIBLE |
        WS_VSCROLL | LBS_NOTIFY,
        15, 115, 230, 400,
        hwnd,
        (HMENU)102,
        nullptr,
        nullptr
    );

    deleteButton = createButton(
        hwnd,
        L"Удалить чат",
        103,
        15, 525, 230, 35
    );

    // Основная область

    titleLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Ёжик AI",
        WS_CHILD | WS_VISIBLE,
        275, 20, 600, 40,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );

    messageView = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE |
        WS_VSCROLL |
        ES_MULTILINE |
        ES_READONLY |
        ES_AUTOVSCROLL,
        275, 75, 700, 430,
        hwnd,
        (HMENU)104,
        nullptr,
        nullptr
    );

    inputBox = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE |
        ES_MULTILINE |
        ES_AUTOVSCROLL |
        WS_VSCROLL,
        275, 520, 560, 65,
        hwnd,
        (HMENU)105,
        nullptr,
        nullptr
    );

    sendButton = createButton(
        hwnd,
        L"Отправить ➤",
        106,
        845, 520, 130, 65
    );

    // Шрифты

    HWND controls[] =
    {
        newChatButton,
        chatList,
        deleteButton,
        titleLabel,
        messageView,
        inputBox,
        sendButton
    };

    for (HWND control : controls)
    {
        SendMessageW(
            control,
            WM_SETFONT,
            (WPARAM)fontMain,
            TRUE
        );
    }

    SendMessageW(
        titleLabel,
        WM_SETFONT,
        (WPARAM)fontTitle,
        TRUE
    );

    refreshChatList();
    refreshMessages();
}

// =====================================================
// ОБРАБОТКА ОКНА
// =====================================================

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
        {
            createInterface(hwnd);
            return 0;
        }

        case WM_COMMAND:
        {
            int id = LOWORD(wParam);
            int event = HIWORD(wParam);

            if (id == 101)
            {
                createChat();
                return 0;
            }

            if (id == 102 && event == LBN_SELCHANGE)
            {
                int selected = (int)SendMessageW(
                    chatList,
                    LB_GETCURSEL,
                    0,
                    0
                );

                if (selected >= 0 &&
                    selected < (int)chats.size())
                {
                    currentChat = selected;
                    refreshMessages();
                }

                return 0;
            }

            if (id == 103)
            {
                if (currentChat >= 0 &&
                    currentChat < (int)chats.size())
                {
                    chats.erase(chats.begin() + currentChat);

                    if (chats.empty())
                        currentChat = -1;
                    else if (currentChat >= (int)chats.size())
                        currentChat = (int)chats.size() - 1;

                    saveChats();
                    refreshChatList();
                    refreshMessages();
                }

                return 0;
            }

            if (id == 106)
            {
                sendMessage();
                return 0;
            }

            // Ctrl + Enter отправляет сообщение.
            if (id == 105 && event == EN_MAXTEXT)
                return 0;

            break;
        }

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        {
            HDC dc = (HDC)wParam;

            SetTextColor(dc, COLOR_TEXT);

            if ((HWND)lParam == chatList)
            {
                SetBkColor(dc, COLOR_SIDEBAR);
                return (LRESULT)sidebarBrush;
            }

            SetBkColor(dc, COLOR_BACKGROUND);

            return (LRESULT)backgroundBrush;
        }

        case WM_CLOSE:
        {
            saveChats();
            DestroyWindow(hwnd);
            return 0;
        }

        case WM_DESTROY:
        {
            saveChats();

            if (fontMain)
                DeleteObject(fontMain);

            if (fontTitle)
                DeleteObject(fontTitle);

            if (backgroundBrush)
                DeleteObject(backgroundBrush);

            if (sidebarBrush)
                DeleteObject(sidebarBrush);

            if (inputBrush)
                DeleteObject(inputBrush);

            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProcW(
        hwnd,
        message,
        wParam,
        lParam
    );
}

// =====================================================
// ЗАПУСК ПРИЛОЖЕНИЯ
// =====================================================

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int showCommand)
{
    INITCOMMONCONTROLSEX controls = {};

    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_STANDARD_CLASSES;

    InitCommonControlsEx(&controls);

    loadChats();

    WNDCLASSW wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"EzhikAIWindow";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(COLOR_BACKGROUND);

    RegisterClassW(&wc);

    mainWindow = CreateWindowExW(
        0,
        L"EzhikAIWindow",
        L"Ёжик AI — Умный помощник",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1030,
        680,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (!mainWindow)
        return 1;

    ShowWindow(mainWindow, showCommand);
    UpdateWindow(mainWindow);

    MSG msg = {};

    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
