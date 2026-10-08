#define UNICODE
#define _UNICODE
#define NOMINMAX

#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cwctype>
#include <algorithm>
#include <iterator>

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")

using namespace std;

// =====================================================
// ЦВЕТА
// =====================================================

const COLORREF EZHIK_BG = RGB(25, 27, 34);
const COLORREF EZHIK_SIDEBAR = RGB(31, 34, 43);
const COLORREF EZHIK_TEXT = RGB(235, 238, 245);
const COLORREF EZHIK_INPUT_BG = RGB(42, 45, 55);

// =====================================================
// ID ЭЛЕМЕНТОВ
// =====================================================

#define ID_CHAT_LIST 101
#define ID_NEW_CHAT 102
#define ID_DELETE 103
#define ID_CHAT_VIEW 104
#define ID_INPUT 105
#define ID_SEND 106

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
HWND titleLabel = nullptr;

HFONT mainFont = nullptr;
HFONT titleFont = nullptr;

HBRUSH backgroundBrush = nullptr;
HBRUSH sidebarBrush = nullptr;
HBRUSH inputBrush = nullptr;

vector<Chat> chats;
int currentChat = -1;

const wchar_t* SAVE_FILE = L"ezhik_chats.dat";

// =====================================================
// СОХРАНЕНИЕ СТРОК
// =====================================================

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

// =====================================================
// СОХРАНЕНИЕ ЧАТОВ
// =====================================================

void saveChats() {
    ofstream file(SAVE_FILE, ios::binary | ios::trunc);

    if (!file) {
        return;
    }

    uint32_t count = static_cast<uint32_t>(
        min<size_t>(chats.size(), 1000)
    );

    file.write(
        reinterpret_cast<const char*>(&count),
        sizeof(count)
    );

    for (uint32_t i = 0; i < count; i++) {
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

    uint32_t count = 0;

    if (!file.read(
        reinterpret_cast<char*>(&count),
        sizeof(count)
    ) || count > 1000) {
        return;
    }

    vector<Chat> loaded;

    for (uint32_t i = 0; i < count; i++) {
        Chat chat;

        if (!readString(file, chat.title)) {
            return;
        }

        uint32_t messageCount = 0;

        if (!file.read(
            reinterpret_cast<char*>(&messageCount),
            sizeof(messageCount)
        ) || messageCount > 10000) {
            return;
        }

        for (uint32_t j = 0; j < messageCount; j++) {
            Message message;

            if (!readString(file, message.author) ||
                !readString(file, message.text)) {
                return;
            }

            chat.messages.push_back(message);
        }

        loaded.push_back(chat);
    }

    chats = loaded;
}

// =====================================================
// ПОИСК КЛЮЧЕВЫХ СЛОВ
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

// =====================================================
// ОТВЕТЫ ЁЖИК AI НА РУССКОМ
// =====================================================

wstring aiAnswer(const wstring& original) {
    wstring text = lowerText(original);

    // Приветствия
    if (contains(text, L"привет") ||
        contains(text, L"здравствуй") ||
        contains(text, L"добрый день") ||
        contains(text, L"доброе утро") ||
        contains(text, L"добрый вечер") ||
        contains(text, L"hello") ||
        contains(text, L"hi") ||
        contains(text, L"hey") ||
        contains(text, L"хай")) {

        return L"Привет! Я Ёжик AI. Чем могу помочь?";
    }

    // Знакомство
    if (contains(text, L"кто ты") ||
        contains(text, L"ты кто") ||
        contains(text, L"твоё имя") ||
        contains(text, L"твое имя") ||
        contains(text, L"who are you")) {

        return L"Я Ёжик AI — твой русскоязычный помощник!";
    }

    // Настроение
    if (contains(text, L"как дела") ||
        contains(text, L"как ты") ||
        contains(text, L"how are you")) {

        return L"У меня всё отлично! А как твои дела?";
    }

    // Благодарность
    if (contains(text, L"спасибо") ||
        contains(text, L"благодарю") ||
        contains(text, L"thank you") ||
        contains(text, L"thanks")) {

        return L"Пожалуйста! Рад помочь!";
    }

    // Прощание
    if (contains(text, L"пока") ||
        contains(text, L"до свидания") ||
        contains(text, L"goodbye") ||
        contains(text, L"bye")) {

        return L"Пока! Возвращайся, когда захочешь пообщаться.";
    }

    // Godot
    if (contains(text, L"godot") ||
        contains(text, L"гадот")) {

        return L"Godot — игровой движок для создания 2D- и "
               L"3D-игр. Для начала изучи сцены, узлы и GDScript.";
    }

    // Смешарики
    if (contains(text, L"смешарик")) {
        return L"Смешарики — отличный мультсериал! "
               L"Какой персонаж тебе нравится больше всего?";
    }

    // Игры
    if (contains(text, L"игр") ||
        contains(text, L"game") ||
        contains(text, L"games")) {

        return L"Игры бывают разных жанров: приключения, "
               L"ужасы, стратегии, гонки и многое другое. "
               L"О какой игре хочешь поговорить?";
    }

    // Программирование
    if (contains(text, L"программ") ||
        contains(text, L"код") ||
        contains(text, L"c++") ||
        contains(text, L"с++") ||
        contains(text, L"python")) {

        return L"Программирование позволяет создавать игры, "
               L"приложения и сайты. Напиши, что именно "
               L"ты хочешь сделать, и я постараюсь помочь.";
    }

    // Математика
    if (contains(text, L"математ") ||
        contains(text, L"посчитай") ||
        contains(text, L"сколько будет") ||
        contains(text, L"calculate")) {

        return L"Я пока не умею надёжно решать все примеры. "
               L"Напиши выражение, и я попробую помочь.";
    }

    // Школа
    if (contains(text, L"школ") ||
        contains(text, L"домашн") ||
        contains(text, L"урок") ||
        contains(text, L"дз")) {

        return L"Давай разберёмся с заданием! "
               L"Напиши предмет и условие задачи.";
    }

    // Помощь
    if (contains(text, L"помоги") ||
        contains(text, L"помощь") ||
        contains(text, L"help")) {

        return L"Конечно! Опиши свою проблему, "
               L"и я постараюсь помочь.";
    }

    // Объяснение
    if (contains(text, L"объясни") ||
        contains(text, L"расскажи") ||
        contains(text, L"что такое") ||
        contains(text, L"почему")) {

        return L"Я попробую объяснить простыми словами. "
               L"Пока моя база знаний ограничена, поэтому "
               L"задай вопрос конкретнее.";
    }

    // Приветствие на английском не должно приводить
    // к английскому ответу: все ответы выше русские.
    return L"Я пока не знаю точного ответа на этот вопрос. "
           L"Моя встроенная база знаний ограничена. "
           L"Попробуй уточнить вопрос, и я постараюсь помочь.";
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
                 L"Создай чат и напиши сообщение.";
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
// СОЗДАНИЕ ЧАТА
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

    if (inputBox) {
        SetFocus(inputBox);
    }
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

    if (MessageBoxW(
        mainWindow,
        L"Удалить выбранный чат и его переписку?",
        L"Ёжик AI",
        MB_YESNO | MB_ICONWARNING
    ) != IDYES) {

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
    if (!inputBox) {
        return;
    }

    int length = GetWindowTextLengthW(inputBox);

    if (length <= 0) {
        return;
    }

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

    if (currentChat < 0 ||
        currentChat >= static_cast<int>(chats.size())) {

        newChat();
    }

    Chat& chat = chats[currentChat];

    if (chat.messages.empty()) {
        chat.title = text.substr(0, 24);

        if (text.size() > 24) {
            chat.title += L"...";
        }
    }

    chat.messages.push_back({ L"Вы", text });
    chat.messages.push_back({ L"Ёжик AI", aiAnswer(text) });

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

    int listHeight = max(
        100,
        height - 55 - buttonHeight * 2 - 55
    );

    MoveWindow(
        titleLabel,
        14, 12, 200, 30,
        TRUE
    );

    MoveWindow(
        chatList,
        margin, 55,
        sidebarWidth - margin * 2,
        listHeight,
        TRUE
    );

    int listBottom = 55 + listHeight;

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
        listBottom + buttonHeight + 16,
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
        max(sidebarWidth + margin, width - 95),
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
    titleLabel = CreateWindowExW(
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
        WS_CHILD | WS_VISIBLE | ES_MULTILINE |
        ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
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
        17, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    titleFont = CreateFontW(
        22, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    HWND controls[] = {
        titleLabel,
        chatList,
        chatView,
        inputBox,
        GetDlgItem(hwnd, ID_NEW_CHAT),
        GetDlgItem(hwnd, ID_DELETE),
        GetDlgItem(hwnd, ID_SEND)
    };

    for (HWND control : controls) {
        if (control) {
            SendMessageW(
                control,
                WM_SETFONT,
                reinterpret_cast<WPARAM>(mainFont),
                TRUE
            );
        }
    }

    SendMessageW(
        titleLabel,
        WM_SETFONT,
        reinterpret_cast<WPARAM>(titleFont),
        TRUE
    );

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

        if (control == titleLabel) {
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

        if (mainFont) DeleteObject(mainFont);
        if (titleFont) DeleteObject(titleFont);
        if (backgroundBrush) DeleteObject(backgroundBrush);
        if (sidebarBrush) DeleteObject(sidebarBrush);
        if (inputBrush) DeleteObject(inputBrush);

        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

// =====================================================
// ЗАПУСК ПРОГРАММЫ
// =====================================================

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int showCommand
) {
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
