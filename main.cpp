#define UNICODE
#define _UNICODE
#define NOMINMAX

#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cwctype>
#include <clocale>
#include <algorithm>

using namespace std;

// =====================================================
// ЦВЕТА
// =====================================================

const COLORREF EZHIK_BG       = RGB(25, 27, 34);
const COLORREF EZHIK_SIDEBAR  = RGB(31, 34, 43);
const COLORREF EZHIK_TEXT     = RGB(235, 238, 245);
const COLORREF EZHIK_GREEN    = RGB(70, 190, 135);
const COLORREF EZHIK_INPUT_BG = RGB(42, 45, 55);

// =====================================================
// ID ЭЛЕМЕНТОВ
// =====================================================

#define ID_CHAT_LIST  101
#define ID_NEW_CHAT   102
#define ID_DELETE     103
#define ID_CHAT_VIEW  104
#define ID_INPUT      105
#define ID_SEND       106

// =====================================================
// СТРУКТУРЫ
// =====================================================

struct Message {
    wstring author;
    wstring text;
};

struct Chat {
    wstring title;
    vector<Message> messages;
};

// =====================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// =====================================================

HWND mainWindow = nullptr;
HWND chatList = nullptr;
HWND chatView = nullptr;
HWND inputBox = nullptr;

HFONT mainFont = nullptr;
HFONT titleFont = nullptr;

HBRUSH backgroundBrush = nullptr;
HBRUSH sidebarBrush = nullptr;
HBRUSH inputBrush = nullptr;

vector<Chat> chats;
int currentChat = -1;

// =====================================================
// СОХРАНЕНИЕ ЧАТОВ
// Файл создаётся автоматически.
// Это бинарный файл, а не TXT.
// =====================================================

const wchar_t* SAVE_FILE = L"ezhik_chats.dat";

void writeString(ofstream& file, const wstring& text) {
    uint32_t length = static_cast<uint32_t>(
        min<size_t>(text.size(), 100000)
    );

    file.write(
        reinterpret_cast<const char*>(&length),
        sizeof(length)
    );

    if (length > 0) {
        file.write(
            reinterpret_cast<const char*>(text.data()),
            length * sizeof(wchar_t)
        );
    }
}

bool readString(ifstream& file, wstring& text) {
    uint32_t length = 0;

    if (!file.read(
        reinterpret_cast<char*>(&length),
        sizeof(length)
    )) {
        return false;
    }

    if (length > 100000) {
        return false;
    }

    text.clear();
    text.resize(length);

    if (length > 0) {
        if (!file.read(
            reinterpret_cast<char*>(&text[0]),
            length * sizeof(wchar_t)
        )) {
            return false;
        }
    }

    return true;
}

void saveChats() {
    ofstream file(SAVE_FILE, ios::binary | ios::trunc);

    if (!file) {
        return;
    }

    uint32_t chatCount = static_cast<uint32_t>(
        min<size_t>(chats.size(), 1000)
    );

    file.write(
        reinterpret_cast<const char*>(&chatCount),
        sizeof(chatCount)
    );

    for (uint32_t i = 0; i < chatCount; i++) {
        writeString(file, chats[i].title);

        uint32_t messageCount = static_cast<uint32_t>(
            min<size_t>(chats[i].messages.size(), 10000)
        );

        file.write(
            reinterpret_cast<const char*>(&messageCount),
            sizeof(messageCount)
        );

        for (uint32_t j = 0; j < messageCount; j++) {
            writeString(file, chats[i].messages[j].author);
            writeString(file, chats[i].messages[j].text);
        }
    }
}

void loadChats() {
    ifstream file(SAVE_FILE, ios::binary);

    if (!file) {
        return;
    }

    uint32_t chatCount = 0;

    if (!file.read(
        reinterpret_cast<char*>(&chatCount),
        sizeof(chatCount)
    )) {
        return;
    }

    if (chatCount > 1000) {
        return;
    }

    vector<Chat> loadedChats;

    for (uint32_t i = 0; i < chatCount; i++) {
        Chat chat;

        if (!readString(file, chat.title)) {
            return;
        }

        uint32_t messageCount = 0;

        if (!file.read(
            reinterpret_cast<char*>(&messageCount),
            sizeof(messageCount)
        )) {
            return;
        }

        if (messageCount > 10000) {
            return;
        }

        for (uint32_t j = 0; j < messageCount; j++) {
            Message message;

            if (!readString(file, message.author)) {
                return;
            }

            if (!readString(file, message.text)) {
                return;
            }

            chat.messages.push_back(message);
        }

        loadedChats.push_back(chat);
    }

    chats = loadedChats;
}

// =====================================================
// ПРОСТЫЕ ОТВЕТЫ ЁЖИК AI
// Это пока не полноценная нейросеть.
// =====================================================

wstring lowerText(wstring text) {
    for (wchar_t& c : text) {
        c = towlower(c);
    }

    return text;
}

bool contains(const wstring& text, const wstring& word) {
    return text.find(word) != wstring::npos;
}

wstring aiAnswer(const wstring& originalText) {
    wstring text = lowerText(originalText);

    if (text.empty()) {
        return L"Напиши что-нибудь, и я попробую ответить!";
    }

    if (contains(text, L"привет") ||
        contains(text, L"здравствуй") ||
        contains(text, L"хай") ||
        contains(text, L"hello") ||
        contains(text, L"hi")) {

        return L"Привет! Я Ёжик AI. Чем могу помочь?";
    }

    if (contains(text, L"как дела") ||
        contains(text, L"как ты")) {

        return L"Всё отлично! Готов общаться с тобой.";
    }

    if (contains(text, L"кто ты") ||
        contains(text, L"ты кто") ||
        contains(text, L"твоё имя") ||
        contains(text, L"твое имя")) {

        return L"Я Ёжик AI — твой чат-помощник!";
    }

    if (contains(text, L"смешарик")) {
        return L"Смешарики — классный мультсериал! "
               L"Какой персонаж тебе нравится больше всего?";
    }

    if (contains(text, L"godot")) {
        return L"Godot — игровой движок. В нём можно создавать "
               L"2D- и 3D-игры. Для начала изучи сцены, узлы "
               L"и GDScript.";
    }

    if (contains(text, L"математ") ||
        contains(text, L"посчитай") ||
        contains(text, L"сколько будет")) {

        return L"Я пока не умею надёжно решать все математические "
               L"задачи. Напиши пример, и я попробую помочь.";
    }

    if (contains(text, L"спасибо")) {
        return L"Пожалуйста! Рад помочь!";
    }

    if (contains(text, L"пока")) {
        return L"Пока! Возвращайся, когда захочешь пообщаться.";
    }

    if (contains(text, L"помоги") ||
        contains(text, L"помощь")) {

        return L"Конечно! Опиши, что нужно сделать, "
               L"и я постараюсь помочь.";
    }

    if (contains(text, L"школ") ||
        contains(text, L"домашн") ||
        contains(text, L"урок")) {

        return L"Давай разберёмся! Напиши предмет и условие задания.";
    }

    if (contains(text, L"кто такой") ||
        contains(text, L"кто такая") ||
        contains(text, L"расскажи про")) {

        return L"Я пока знаю не всё. Уточни вопрос, "
               L"и я попробую ответить.";
    }

    return L"Я пока не знаю точного ответа на этот вопрос. "
           L"Моя база знаний ограничена, но мы можем "
           L"постепенно улучшать Ёжик AI!";
}

// =====================================================
// ОБНОВЛЕНИЕ СПИСКА ЧАТОВ
// =====================================================

void updateChatList() {
    if (!chatList) {
        return;
    }

    SendMessageW(chatList, LB_RESETCONTENT, 0, 0);

    for (const Chat& chat : chats) {
        SendMessageW(
            chatList,
            LB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(chat.title.c_str())
        );
    }

    if (currentChat >= 0 &&
        currentChat < static_cast<int>(chats.size())) {

        SendMessageW(
            chatList,
            LB_SETCURSEL,
            currentChat,
            0
        );
    }
}

// =====================================================
// ОБНОВЛЕНИЕ ПЕРЕПИСКИ
// =====================================================

void updateChatView() {
    if (!chatView) {
        return;
    }

    wstring output;

    if (currentChat < 0 ||
        currentChat >= static_cast<int>(chats.size())) {

        output = L"Добро пожаловать в Ёжик AI!\r\n\r\n"
                 L"Создай чат слева и напиши своё сообщение.";
    }
    else {
        const Chat& chat = chats[currentChat];

        if (chat.messages.empty()) {
            output = L"Ёжик AI\r\n\r\n"
                     L"Привет! Напиши первое сообщение.";
        }

        for (const Message& message : chat.messages) {
            output += message.author;
            output += L":\r\n";
            output += message.text;
            output += L"\r\n\r\n";
        }
    }

    SetWindowTextW(chatView, output.c_str());

    SendMessageW(
        chatView,
        EM_SETSEL,
        static_cast<WPARAM>(output.size()),
        static_cast<LPARAM>(output.size())
    );

    SendMessageW(chatView, EM_SCROLLCARET, 0, 0);
}

// =====================================================
// СОЗДАНИЕ НОВОГО ЧАТА
// =====================================================

void newChat() {
    Chat chat;

    chat.title = L"Новый чат " +
        to_wstring(chats.size() + 1);

    chats.push_back(chat);

    currentChat = static_cast<int>(chats.size()) - 1;

    updateChatList();
    updateChatView();
    saveChats();

    SetFocus(inputBox);
}

// =====================================================
// УДАЛЕНИЕ ЧАТА
// =====================================================

void deleteChat() {
    if (currentChat < 0 ||
        currentChat >= static_cast<int>(chats.size())) {

        MessageBoxW(
            mainWindow,
            L"Сначала выбери чат.",
            L"Ёжик AI",
            MB_OK | MB_ICONINFORMATION
        );

        return;
    }

    int result = MessageBoxW(
        mainWindow,
        L"Удалить выбранный чат и всю его переписку?",
        L"Удаление чата",
        MB_YESNO | MB_ICONWARNING
    );

    if (result != IDYES) {
        return;
    }

    chats.erase(chats.begin() + currentChat);

    if (chats.empty()) {
        currentChat = -1;
    }
    else if (currentChat >= static_cast<int>(chats.size())) {
        currentChat = static_cast<int>(chats.size()) - 1;
    }

    updateChatList();
    updateChatView();
    saveChats();
}

// =====================================================
// ОТПРАВКА СООБЩЕНИЯ
// =====================================================

void sendMessage() {
    if (currentChat < 0 ||
        currentChat >= static_cast<int>(chats.size())) {

        newChat();
    }

    int length = GetWindowTextLengthW(inputBox);

    if (length <= 0) {
        return;
    }

    // Выделяем место и для завершающего нулевого символа.
    wstring text(static_cast<size_t>(length) + 1, L'\0');

    GetWindowTextW(
        inputBox,
        &text[0],
        length + 1
    );

    text.resize(wcslen(text.c_str()));

    if (text.empty()) {
        return;
    }

    // Первое сообщение становится названием чата.
    if (chats[currentChat].messages.empty()) {
        wstring title = text;

        if (title.size() > 24) {
            title = title.substr(0, 24) + L"...";
        }

        chats[currentChat].title = title;
    }

    Message userMessage;
    userMessage.author = L"Вы";
    userMessage.text = text;

    chats[currentChat].messages.push_back(userMessage);

    Message botMessage;
    botMessage.author = L"Ёжик AI";
    botMessage.text = aiAnswer(text);

    chats[currentChat].messages.push_back(botMessage);

    SetWindowTextW(inputBox, L"");

    updateChatList();
    updateChatView();
    saveChats();

    SetFocus(inputBox);
}

// =====================================================
// РАЗМЕЩЕНИЕ ЭЛЕМЕНТОВ
// =====================================================

void layoutInterface(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    int width = rect.right;
    int height = rect.bottom;

    const int sidebarWidth = 225;
    const int margin = 12;
    const int buttonHeight = 36;
    const int inputHeight = 42;

    MoveWindow(
        chatList,
        margin,
        55,
        sidebarWidth - margin * 2,
        max(100, height - 55 - buttonHeight * 2 - 55),
        TRUE
    );

    int listBottom = height - 55 - buttonHeight * 2;

    MoveWindow(
        GetDlgItem(hwnd, ID_NEW_CHAT),
        margin,
        listBottom + 8,
        sidebarWidth - margin * 2,
        buttonHeight,
        TRUE
    );

    MoveWindow(
        GetDlgItem(hwnd, ID_DELETE),
        margin,
        listBottom + 8 + buttonHeight + 8,
        sidebarWidth - margin * 2,
        buttonHeight,
        TRUE
    );

    MoveWindow(
        chatView,
        sidebarWidth + margin,
        48,
        max(100, width - sidebarWidth - margin * 2),
        max(100, height - inputHeight - 75),
        TRUE
    );

    MoveWindow(
        inputBox,
        sidebarWidth + margin,
        height - inputHeight - margin,
        max(100, width - sidebarWidth - 120),
        inputHeight,
        TRUE
    );

    MoveWindow(
        GetDlgItem(hwnd, ID_SEND),
        width - 95,
        height - inputHeight - margin,
        80,
        inputHeight,
        TRUE
    );
}

// =====================================================
// СОЗДАНИЕ ИНТЕРФЕЙСА
// =====================================================

void createInterface(HWND hwnd) {
    chatList = CreateWindowExW(
        0,
        L"LISTBOX",
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(ID_CHAT_LIST),
        GetModuleHandleW(nullptr),
        nullptr
    );

    CreateWindowExW(
        0,
        L"STATIC",
        L"Ёжик AI",
        WS_CHILD | WS_VISIBLE,
        14, 12, 200, 30,
        hwnd,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );

    CreateWindowExW(
        0,
        L"BUTTON",
        L"+ Новый чат",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(ID_NEW_CHAT),
        GetModuleHandleW(nullptr),
        nullptr
    );

    CreateWindowExW(
        0,
        L"BUTTON",
        L"Удалить чат",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(ID_DELETE),
        GetModuleHandleW(nullptr),
        nullptr
    );

    chatView = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE |
        ES_MULTILINE | ES_AUTOVSCROLL |
        ES_READONLY | WS_VSCROLL,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(ID_CHAT_VIEW),
        GetModuleHandleW(nullptr),
        nullptr
    );

    inputBox = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_AUTOHSCROLL,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(ID_INPUT),
        GetModuleHandleW(nullptr),
        nullptr
    );

    CreateWindowExW(
        0,
        L"BUTTON",
        L"Отправить",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(ID_SEND),
        GetModuleHandleW(nullptr),
        nullptr
    );

    mainFont = CreateFontW(
        17, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    titleFont = CreateFontW(
        22, 0, 0, 0,
        FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    HWND controls[] = {
        chatList,
        chatView,
        inputBox,
        GetDlgItem(hwnd, ID_NEW_CHAT),
        GetDlgItem(hwnd, ID_DELETE),
        GetDlgItem(hwnd, ID_SEND)
    };

    for (HWND control : controls) {
        SendMessageW(
            control,
            WM_SETFONT,
            reinterpret_cast<WPARAM>(mainFont),
            TRUE
        );
    }

    HWND title = GetWindow(hwnd, GW_CHILD);

    while (title) {
        wchar_t name[64] = {};
        GetClassNameW(title, name, 64);

        if (wstring(name) == L"Static") {
            SendMessageW(
                title,
                WM_SETFONT,
                reinterpret_cast<WPARAM>(titleFont),
                TRUE
            );
            break;
        }

        title = GetWindow(title, GW_HWNDNEXT);
    }

    layoutInterface(hwnd);
    updateChatList();
    updateChatView();
}

// =====================================================
// ОБРАБОТЧИК ОКНА
// =====================================================

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    switch (message) {

    case WM_CREATE:
        createInterface(hwnd);
        return 0;

    case WM_SIZE:
        if (chatList) {
            layoutInterface(hwnd);
        }
        return 0;

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int notification = HIWORD(wParam);

        if (id == ID_NEW_CHAT) {
            newChat();
            return 0;
        }

        if (id == ID_DELETE) {
            deleteChat();
            return 0;
        }

        if (id == ID_SEND) {
            sendMessage();
            return 0;
        }

        if (id == ID_CHAT_LIST &&
            notification == LBN_SELCHANGE) {

            int selected = static_cast<int>(
                SendMessageW(chatList, LB_GETCURSEL, 0, 0)
            );

            if (selected >= 0 &&
                selected < static_cast<int>(chats.size())) {

                currentChat = selected;
                updateChatView();
            }

            return 0;
        }

        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);

        SetTextColor(hdc, EZHIK_TEXT);
        SetBkMode(hdc, OPAQUE);

        if (control == GetWindow(hwnd, GW_CHILD)) {
            SetBkColor(hdc, EZHIK_SIDEBAR);
            return reinterpret_cast<LRESULT>(sidebarBrush);
        }

        SetBkColor(hdc, EZHIK_BG);
        return reinterpret_cast<LRESULT>(backgroundBrush);
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);

        SetTextColor(hdc, EZHIK_TEXT);

        if (control == inputBox) {
            SetBkColor(hdc, EZHIK_INPUT_BG);
            return reinterpret_cast<LRESULT>(inputBrush);
        }

        SetBkColor(hdc, EZHIK_BG);
        return reinterpret_cast<LRESULT>(backgroundBrush);
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdc = reinterpret_cast<HDC>(wParam);

        SetTextColor(hdc, EZHIK_TEXT);
        SetBkColor(hdc, EZHIK_SIDEBAR);

        return reinterpret_cast<LRESULT>(sidebarBrush);
    }

    case WM_ERASEBKGND: {
        HDC hdc = reinterpret_cast<HDC>(wParam);

        RECT rect;
        GetClientRect(hwnd, &rect);

        FillRect(hdc, &rect, backgroundBrush);

        RECT sidebar = rect;
        sidebar.right = 225;

        FillRect(hdc, &sidebar, sidebarBrush);

        return 1;
    }

    case WM_DESTROY:
        saveChats();

        if (mainFont) {
            DeleteObject(mainFont);
        }

        if (titleFont) {
            DeleteObject(titleFont);
        }

        if (backgroundBrush) {
            DeleteObject(backgroundBrush);
        }

        if (sidebarBrush) {
            DeleteObject(sidebarBrush);
        }

        if (inputBrush) {
            DeleteObject(inputBrush);
        }

        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

// =====================================================
// ТОЧКА ВХОДА
// =====================================================

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int showCommand
) {
    setlocale(LC_ALL, "");

    backgroundBrush = CreateSolidBrush(EZHIK_BG);
    sidebarBrush = CreateSolidBrush(EZHIK_SIDEBAR);
    inputBrush = CreateSolidBrush(EZHIK_INPUT_BG);

    loadChats();

    WNDCLASSW wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"EzhikAIWindow";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = backgroundBrush;

    if (!RegisterClassW(&wc)) {
        MessageBoxW(
            nullptr,
            L"Не удалось зарегистрировать окно.",
            L"Ёжик AI",
            MB_OK | MB_ICONERROR
        );

        return 1;
    }

    mainWindow = CreateWindowExW(
        0,
        L"EzhikAIWindow",
        L"Ёжик AI",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1000,
        700,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (!mainWindow) {
        MessageBoxW(
            nullptr,
            L"Не удалось создать окно программы.",
            L"Ёжик AI",
            MB_OK | MB_ICONERROR
        );

        DeleteObject(backgroundBrush);
        DeleteObject(sidebarBrush);
        DeleteObject(inputBrush);

        return 1;
    }

    ShowWindow(mainWindow, showCommand);
    UpdateWindow(mainWindow);

    if (chats.empty()) {
        newChat();
    }

    MSG msg = {};

    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
