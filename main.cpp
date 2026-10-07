// ============================================================
//                 ЁЖИК AI v1.0
//              C++17 / Windows / Console
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <random>
#include <chrono>
#include <iomanip>

using namespace std;

// ============================================================
// УТИЛИТЫ
// ============================================================

string lowerStr(string s)
{
    transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)tolower(c); });
    return s;
}

bool contains(const string& text, const string& word)
{
    return lowerStr(text).find(lowerStr(word)) != string::npos;
}

vector<string> split(const string& s)
{
    vector<string> result;
    string word;

    for (char c : s)
    {
        if (isspace((unsigned char)c))
        {
            if (!word.empty())
            {
                result.push_back(word);
                word.clear();
            }
        }
        else
            word += c;
    }

    if (!word.empty())
        result.push_back(word);

    return result;
}

// ============================================================
// КАЛЬКУЛЯТОР
// ============================================================

class Calculator
{
private:

    string expr;
    size_t pos = 0;

    void skip()
    {
        while (pos < expr.size() &&
               isspace((unsigned char)expr[pos]))
            pos++;
    }

    double number()
    {
        skip();

        size_t start = pos;

        while (pos < expr.size() &&
               (isdigit((unsigned char)expr[pos]) ||
                expr[pos] == '.'))
        {
            pos++;
        }

        if (start == pos)
            throw runtime_error("number expected");

        return stod(expr.substr(start, pos - start));
    }

    double factor()
    {
        skip();

        if (pos < expr.size() && expr[pos] == '(')
        {
            pos++;
            double x = expression();
            skip();

            if (pos >= expr.size() || expr[pos] != ')')
                throw runtime_error("missing )");

            pos++;
            return x;
        }

        if (pos < expr.size() && expr[pos] == '-')
        {
            pos++;
            return -factor();
        }

        return number();
    }

    double term()
    {
        double x = factor();

        while (true)
        {
            skip();

            if (pos >= expr.size())
                break;

            char op = expr[pos];

            if (op != '*' && op != '/')
                break;

            pos++;

            double y = factor();

            if (op == '*')
                x *= y;
            else
            {
                if (y == 0)
                    throw runtime_error("division by zero");

                x /= y;
            }
        }

        return x;
    }

    double expression()
    {
        double x = term();

        while (true)
        {
            skip();

            if (pos >= expr.size())
                break;

            char op = expr[pos];

            if (op != '+' && op != '-')
                break;

            pos++;

            double y = term();

            if (op == '+')
                x += y;
            else
                x -= y;
        }

        return x;
    }

public:

    double calculate(const string& input)
    {
        expr = input;
        pos = 0;

        double result = expression();

        skip();

        if (pos != expr.size())
            throw runtime_error("invalid expression");

        return result;
    }
};

// ============================================================
// БАЗА ЗНАНИЙ
// ============================================================

struct Knowledge
{
    string key;
    string answer;
};

class KnowledgeBase
{
private:

    vector<Knowledge> data;

public:

    KnowledgeBase()
    {
        // ----------------------------------------------------
        // ЁЖИК
        // ----------------------------------------------------

        add("ежик",
            "Я Ёжик 🦔 — твой виртуальный помощник!");

        add("кто ты",
            "Я Ёжик AI 🦔 — программа на C++, которая умеет общаться, считать и искать ответы в своей базе знаний.");

        add("любимый мультфильм",
            "Мой любимый мультфильм — «Смешарики»! 🦔❤️");

        add("смешарики",
            "«Смешарики» — российский анимационный сериал про необычных круглых персонажей. Среди известных героев: Крош, Ёжик, Нюша, Бараш, Лосяш, Копатыч, Совунья и Кар-Карыч.");

        add("крош",
            "Крош — энергичный, весёлый и очень активный персонаж «Смешариков». 🐰");

        add("нюша",
            "Нюша — героиня «Смешариков», которая любит красоту, внимание и разные интересные занятия. 🐷");

        add("бараш",
            "Бараш — поэт и мечтатель из «Смешариков». Он часто размышляет о жизни. 🐑");

        add("лоша",
            "Лосяш — умный и увлечённый наукой персонаж. 🫎");

        // ----------------------------------------------------
        // ИСТОРИЯ
        // ----------------------------------------------------

        add("древний египет",
            "Древний Египет возник в долине Нила. Египтяне строили пирамиды, использовали иероглифы и создали сложную государственную систему.");

        add("древний рим",
            "Древний Рим был одной из крупнейших держав древнего мира. Римская культура сильно повлияла на Европу.");

        add("древняя греция",
            "Древняя Греция известна философией, театром, Олимпийскими играми и развитием математики.");

        add("вторая мировая война",
            "Вторая мировая война продолжалась с 1939 по 1945 год. Это крупнейший вооружённый конфликт XX века.");

        add("первая мировая война",
            "Первая мировая война проходила в 1914–1918 годах.");

        add("петр первый",
            "Пётр I — российский царь и первый российский император. Он провёл крупные реформы.");

        // ----------------------------------------------------
        // ШКОЛА
        // ----------------------------------------------------

        add("математика",
            "Математика изучает числа, величины, структуры, пространство и закономерности.");

        add("алгебра",
            "Алгебра изучает выражения, уравнения, функции и другие математические объекты.");

        add("геометрия",
            "Геометрия изучает фигуры, их свойства, размеры и взаимное расположение.");

        add("физика",
            "Физика изучает явления природы, движение, энергию, взаимодействия, электричество и многое другое.");

        add("химия",
            "Химия изучает вещества, их строение, свойства и превращения.");

        add("биология",
            "Биология изучает живые организмы и процессы, происходящие в них.");

        add("информатика",
            "Информатика изучает информацию, алгоритмы, компьютеры и способы обработки данных.");

        add("русский язык",
            "Русский язык изучает правила языка, орфографию, пунктуацию, грамматику и культуру речи.");

        add("литература",
            "Литература изучает художественные произведения, их авторов, героев и особенности.");

        add("география",
            "География изучает Землю, природу, население, страны и природные процессы.");

        add("обществознание",
            "Обществознание изучает общество, человека, экономику, право и государство.");

        // ----------------------------------------------------
        // ИГРЫ
        // ----------------------------------------------------

        add("minecraft",
            "Minecraft — игра-песочница, где игрок исследует мир, добывает ресурсы, строит сооружения и может сражаться с существами.");

        add("fortnite",
            "Fortnite — игра с несколькими режимами, включая популярный режим Battle Royale.");

        add("godot",
            "Godot — игровой движок с открытым исходным кодом. На нём можно создавать 2D и 3D игры.");

        add("unity",
            "Unity — игровой движок для создания 2D и 3D игр.");

        add("unreal engine",
            "Unreal Engine — мощный игровой движок, используемый для создания современных 3D игр.");

        add("steam",
            "Steam — цифровая платформа для покупки, загрузки и запуска компьютерных игр.");

        // ----------------------------------------------------
        // ПРОГРАММЫ
        // ----------------------------------------------------

        add("c++",
            "C++ — мощный язык программирования, используемый для игр, приложений, системного ПО и многих других задач.");

        add("python",
            "Python — популярный язык программирования, известный простым синтаксисом и большим количеством библиотек.");

        add("visual studio code",
            "Visual Studio Code — редактор исходного кода с поддержкой множества языков программирования.");

        add("telegram",
            "Telegram — приложение для обмена сообщениями, звонков, групп, каналов и ботов.");

        // ----------------------------------------------------
        // МУЛЬТФИЛЬМЫ
        // ----------------------------------------------------

        add("том и джерри",
            "«Том и Джерри» — мультсериал о коте Томе и мышонке Джерри.");

        add("губка боб",
            "«Губка Боб Квадратные Штаны» — мультсериал о Губке Бобе и жителях Бикини-Боттом.");

        add("фиксики",
            "«Фиксики» — мультсериал о маленьких существах, которые живут рядом с техникой и знают, как она работает.");

        // ----------------------------------------------------
        // ЯЗЫКИ
        // ----------------------------------------------------

        add("hello",
            "Hello! 👋 Я Ёжик AI.");

        add("hi",
            "Hi! 🦔");

        add("привет",
            "Привет! 🦔 Как дела?");

        add("как дела",
            "Отлично! 😎 Готов отвечать на вопросы.");

        add("спасибо",
            "Пожалуйста! ❤️");

        add("пока",
            "Пока! 👋 Возвращайся!");

        add("что умеешь",
            "Я умею общаться, считать, искать информацию в своей базе, отвечать на школьные вопросы, рассказывать об играх, мультфильмах, истории и программировании.");
    }

    void add(const string& key, const string& answer)
    {
        data.push_back({ key, answer });
    }

    string search(const string& question)
    {
        string q = lowerStr(question);

        // Точное совпадение / ключевые слова
        for (const auto& item : data)
        {
            if (q == item.key)
                return item.answer;
        }

        // Поиск по словам
        for (const auto& item : data)
        {
            if (q.find(item.key) != string::npos)
                return item.answer;
        }

        return "";
    }
};

// ============================================================
// ПАМЯТЬ
// ============================================================

class Memory
{
private:

    vector<pair<string, string>> history;

public:

    void add(const string& user, const string& bot)
    {
        history.push_back({ user, bot });

        // Не позволяем памяти бесконечно расти
        if (history.size() > 100)
            history.erase(history.begin());
    }

    void save()
    {
        ofstream file("ezhik_memory.txt");

        if (!file)
            return;

        for (auto& h : history)
        {
            file << "USER|" << h.first << "\n";
            file << "BOT|" << h.second << "\n";
        }
    }

    void load()
    {
        ifstream file("ezhik_memory.txt");

        if (!file)
            return;

        string line;

        while (getline(file, line))
        {
            if (line.rfind("USER|", 0) == 0)
            {
                string user = line.substr(5);

                if (getline(file, line) &&
                    line.rfind("BOT|", 0) == 0)
                {
                    string bot = line.substr(4);
                    history.push_back({ user, bot });
                }
            }
        }
    }

    string lastUserMessage()
    {
        if (history.empty())
            return "";

        return history.back().first;
    }

    int size()
    {
        return (int)history.size();
    }
};

// ============================================================
// ЁЖИК AI
// ============================================================

class EzhikAI
{
private:

    KnowledgeBase knowledge;
    Calculator calculator;
    Memory memory;

    mt19937 rng;

    vector<string> randomAnswers =
    {
        "Интересный вопрос! 🦔",
        "Хмм... Сейчас подумаю 🤔",
        "Я понял тебя! 😎",
        "Хороший вопрос 👍",
        "Давай разберёмся вместе 🦔",
        "Интересно! 👀"
    };

public:

    EzhikAI()
    {
        rng.seed(
            (unsigned)chrono::high_resolution_clock::
            now().time_since_epoch().count()
        );

        memory.load();
    }

    string randomPhrase()
    {
        uniform_int_distribution<int> dist(
            0,
            (int)randomAnswers.size() - 1
        );

        return randomAnswers[dist(rng)];
    }

    // --------------------------------------------------------
    // ПОПЫТКА ПОНЯТЬ ОШИБКИ
    // --------------------------------------------------------

    string normalize(string q)
    {
        q = lowerStr(q);

        // Частые ошибки
        vector<pair<string, string>> fixes =
        {
            {"пра ", "про "},
            {"почемута", "почему-то"},
            {"щас", "сейчас"},
            {"кароче", "короче"},
            {"ваще", "вообще"},
            {"смешарикии", "смешарики"},
            {"ежык", "ежик"},
            {"ежикк", "ежик"},
            {"матемотика", "математика"},
            {"русскийй", "русский"},
            {"превет", "привет"},
            {"приветт", "привет"},
            {"спосибо", "спасибо"}
        };

        for (auto& f : fixes)
        {
            size_t p;

            while ((p = q.find(f.first)) != string::npos)
                q.replace(p, f.first.length(), f.second);
        }

        return q;
    }

    // --------------------------------------------------------
    // КАЛЬКУЛЯТОР
    // --------------------------------------------------------

    bool looksLikeMath(const string& q)
    {
        bool hasNumber = false;

        for (char c : q)
        {
            if (isdigit((unsigned char)c))
                hasNumber = true;

            if (isalpha((unsigned char)c) &&
                (unsigned char)c < 128)
                return false;
        }

        return hasNumber &&
               (q.find('+') != string::npos ||
                q.find('-') != string::npos ||
                q.find('*') != string::npos ||
                q.find('/') != string::npos ||
                q.find('(') != string::npos);
    }

    string calculate(const string& q)
    {
        try
        {
            double result = calculator.calculate(q);

            ostringstream out;

            out << fixed << setprecision(6) << result;

            string s = out.str();

            while (s.back() == '0')
                s.pop_back();

            if (s.back() == '.')
                s.pop_back();

            return "🧮 Ответ: " + s;
        }
        catch (...)
        {
            return "❌ Не получилось посчитать это выражение.";
        }
    }

    // --------------------------------------------------------
    // КОМАНДЫ
    // --------------------------------------------------------

    string command(const string& q)
    {
        if (q == "/help")
        {
            return
                "📚 Команды:\n"
                "/help — список команд\n"
                "/calc — калькулятор\n"
                "/memory — размер памяти\n"
                "/clear — очистить экран\n"
                "/about — информация обо мне\n"
                "/save — сохранить память\n"
                "/exit — выйти";
        }

        if (q == "/about")
        {
            return
                "🦔 Ёжик AI\n"
                "Версия: 1.0\n"
                "Язык: C++17\n"
                "Режим: офлайн\n"
                "Создан как учебный проект ИИ.";
        }

        if (q == "/memory")
        {
            return "🧠 В памяти записей: " +
                   to_string(memory.size());
        }

        if (q == "/save")
        {
            memory.save();
            return "💾 Память сохранена.";
        }

        if (q == "/clear")
        {
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif
            return "🦔 Экран очищен.";
        }

        return "";
    }

    // --------------------------------------------------------
    // ГЛАВНЫЙ ОТВЕТ
    // --------------------------------------------------------

    string answer(string question)
    {
        string q = normalize(question);

        if (q.empty())
            return "🦔 Напиши что-нибудь.";

        // Команды
        if (q[0] == '/')
        {
            string cmd = command(q);

            if (!cmd.empty())
                return cmd;
        }

        // Выход
        if (q == "выход" ||
            q == "exit" ||
            q == "quit")
        {
            return "__EXIT__";
        }

        // Математика
        if (looksLikeMath(q))
            return calculate(q);

        // База знаний
        string result = knowledge.search(q);

        if (!result.empty())
            return result;

        // Специальные вопросы
        if (contains(q, "ты живой") ||
            contains(q, "ты настоящий"))
        {
            return
                "Я не живой человек. Я программа 🤖🦔, "
                "но могу общаться с тобой.";
        }

        if (contains(q, "ты меня любишь"))
        {
            return
                "Я отношусь к тебе дружелюбно ❤️🦔 "
                "и всегда готов помочь!";
        }

        if (contains(q, "тебе нравится"))
        {
            return
                "Мне нравятся интересные вопросы, программирование "
                "и, конечно, «Смешарики»! 🦔❤️";
        }

        // Если ничего не нашли
        return randomPhrase() +
               "\nПока я не нашёл точного ответа в своей базе знаний. "
               "Попробуй задать вопрос немного иначе.";
    }
};

// ============================================================
// ИНТЕРФЕЙС
// ============================================================

void printLogo()
{
    cout << "\n";
    cout << "============================================================\n";
    cout << "                    🦔 ЁЖИК AI\n";
    cout << "============================================================\n";
    cout << "        Умный помощник на C++17\n";
    cout << "============================================================\n";
    cout << "\n";
}

int main()
{
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    printLogo();

    cout << "🦔 Ёжик: Привет! Я Ёжик AI!\n";
    cout << "🦔 Ёжик: Напиши /help, чтобы увидеть команды.\n";
    cout << "🦔 Ёжик: Можешь просто написать мне вопрос.\n\n";

    EzhikAI ai;

    while (true)
    {
        cout << "Ты: ";

        string input;

        getline(cin, input);

        if (cin.fail())
            break;

        string response = ai.answer(input);

        if (response == "__EXIT__")
        {
            cout << "🦔 Ёжик: Пока! 👋\n";
            ai.answer("/save");
            break;
        }

        cout << "\n🦔 Ёжик: "
             << response
             << "\n\n";
    }

    return 0;
}
