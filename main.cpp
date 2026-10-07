// Ёжик — чат-бот на C++17 (русский + английский, калькулятор, смайлики, исправление опечаток)
// Сборка: g++ -std=c++17 -O2 yozhik.cpp -o yozhik   Запуск: ./yozhik
#include <bits/stdc++.h>
using namespace std;
typedef u32string U;

// ---------- UTF-8 и нормализация ----------
U dec(const string& s) {
    U r;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = s[i];
        char32_t cp; int n;
        if (c < 0x80) { cp = c; n = 1; }
        else if ((c >> 5) == 6) { cp = c & 31; n = 2; }
        else if ((c >> 4) == 14) { cp = c & 15; n = 3; }
        else { cp = c & 7; n = 4; }
        for (int k = 1; k < n && i + k < s.size(); k++) cp = (cp << 6) | (s[i + k] & 63);
        r += cp; i += n;
    }
    return r;
}
char32_t low(char32_t c) {
    if (c >= 0x410 && c <= 0x42F) return c + 32;
    if (c == 0x401 || c == 0x451) return 0x435;           // Ё/ё -> е
    if (c < 128) return tolower((int)c);
    return c;
}
bool isWordCh(char32_t c) { return (c < 128 && isalnum((int)c)) || (c >= 0x430 && c <= 0x44F); }
bool isCyr(char32_t c) { return c >= 0x430 && c <= 0x44F; }

vector<U> tokens(const string& s) {
    vector<U> t; U cur;
    for (char32_t c : dec(s)) {
        c = low(c);
        if (isWordCh(c)) cur += c;
        else if (!cur.empty()) { t.push_back(cur); cur.clear(); }
    }
    if (!cur.empty()) t.push_back(cur);
    return t;
}
size_t lev(const U& a, const U& b) {
    vector<size_t> p(b.size() + 1), q(b.size() + 1);
    iota(p.begin(), p.end(), 0);
    for (size_t i = 1; i <= a.size(); i++) {
        q[0] = i;
        for (size_t j = 1; j <= b.size(); j++)
            q[j] = min({p[j] + 1, q[j - 1] + 1, p[j - 1] + (a[i - 1] != b[j - 1])});
        swap(p, q);
    }
    return p[b.size()];
}
// слово w «похоже» на ключ k (учитываем окончания и опечатки)
bool like(const U& w, const U& k) {
    if (k.size() < 4) return w == k;
    if (w.size() + 1 < k.size()) return false;
    U pre = w.substr(0, min(w.size(), k.size()));
    return lev(pre, k) <= (k.size() >= 7 ? 2u : 1u);
}

// ---------- Калькулятор ----------
struct Calc {
    string s; size_t i = 0; bool ok = true, hasOp = false;
    void ws() { while (i < s.size() && s[i] == ' ') i++; }
    double expr() {
        double v = term();
        for (ws(); i < s.size() && (s[i] == '+' || s[i] == '-'); ws()) {
            char o = s[i++]; hasOp = true; double r = term(); v = (o == '+') ? v + r : v - r;
        }
        return v;
    }
    double term() {
        double v = pw();
        for (ws(); i < s.size() && (s[i] == '*' || s[i] == '/'); ws()) {
            char o = s[i++]; hasOp = true; double r = pw();
            if (o == '/' && r == 0) { ok = false; return 0; }
            v = (o == '*') ? v * r : v / r;
        }
        return v;
    }
    double pw() {
        double b = un(); ws();
        if (i < s.size() && s[i] == '^') { i++; hasOp = true; return pow(b, pw()); }
        return b;
    }
    double un() {
        ws();
        if (i < s.size() && s[i] == '-') { i++; return -un(); }
        if (i < s.size() && s[i] == '(') {
            i++; double v = expr(); ws();
            if (i < s.size() && s[i] == ')') i++; else ok = false;
            return v;
        }
        size_t st = i;
        while (i < s.size() && (isdigit((unsigned char)s[i]) || s[i] == '.')) i++;
        if (st == i) { ok = false; return 0; }
        return atof(s.substr(st, i - st).c_str());
    }
};
bool tryCalc(const string& in, string& out) {
    string f;
    for (char c : in) {
        if (c == ',') c = '.';
        if (strchr("0123456789+-*/^().", c)) f += c;
        else if (c == ' ') f += ' ';
    }
    // × и ÷
    if (in.find("\xC3\x97") != string::npos) { for (auto& c : f) (void)c; }
    Calc k; k.s = f;
    double v = k.expr(); k.ws();
    if (!k.ok || !k.hasOp || k.i != k.s.size()) return false;
    ostringstream o;
    if (fabs(v - llround(v)) < 1e-9 && fabs(v) < 1e15) o << llround(v);
    else o << setprecision(10) << v;
    out = o.str();
    return true;
}

// ---------- База знаний ----------
struct Entry { vector<U> keys; string ru, en; };
vector<Entry> kb;
void add(initializer_list<const char*> k, string ru, string en = "") {
    Entry e; for (auto x : k) e.keys.push_back(tokens(x)[0]);
    e.ru = ru; e.en = en.empty() ? ru : en; kb.push_back(e);
}
void initKB() {
    add({"привет", "здравств", "хай", "hello", "hi", "hey"},
        "Привет! Я Ёжик 🦔 Спрашивай что угодно!", "Hi! I'm Yozhik the hedgehog 🦔 Ask me anything!");
    add({"смешарик", "smeshariki", "ежик", "нюша", "крош", "лосяш", "бараш", "копатыч", "совунья", "пин"},
        "Смешарики — мой любимый мультик! 🥰 Это дружная компания круглых героев: Крош, Ёжик, Нюша, Бараш, "
        "Лосяш, Совунья, Копатыч и Пин. Они живут на Круглой поляне, дружат, ссорятся, мирятся и учатся "
        "понимать друг друга. Ёжик — самый добрый и мечтательный, а я назван в его честь 🦔💚",
        "Smeshariki is my favorite cartoon! 🥰 Round friends (Krosh, Hedgehog, Nyusha, Barash, Losyash, "
        "Sovunya, Kopatych, Pin) live on a round meadow and learn about friendship and life 🦔");
    add({"мультик", "мультфильм", "cartoon"},
        "Я знаю много мультиков: Смешарики, Маша и Медведь, Фиксики, Простоквашино, Ну погоди! 🎬 "
        "Но Смешарики — самый любимый! Про какой рассказать?",
        "I know many cartoons: Smeshariki, Masha and the Bear, Fixies... but Smeshariki is my favorite! 🎬");
    add({"маша", "медвед"},
        "«Маша и Медведь» — про непоседливую девочку Машу и доброго Медведя, который терпит её проказы 🐻",
        "'Masha and the Bear' is about a mischievous girl and a kind bear 🐻");
    add({"фиксик"},
        "«Фиксики» — маленькие человечки, которые чинят технику и объясняют, как всё устроено 🔧",
        "'Fixies' are tiny helpers who fix gadgets and explain how things work 🔧");
    add({"любишь", "любовь", "love"},
        "Я тебя люблю! 💖🦔 Ты хороший друг!", "I love you too! 💖🦔 You're a great friend!");
    add({"спасибо", "thanks", "thank"}, "Всегда пожалуйста! 😊", "You're welcome! 😊");
    add({"пока", "bye", "goodbye"}, "Пока-пока! Заходи ещё! 👋🦔", "Bye-bye! Come back soon! 👋🦔");
    add({"дела", "самочувствие", "how"}, "Отлично! 😄 А у тебя как?", "I'm great! 😄 How are you?");
    add({"зовут", "name", "кто", "who"}, "Я Ёжик — умный ИИ-друг и помощник по урокам 🦔📚",
        "I'm Yozhik — a smart AI friend and homework helper 🦔📚");
    // школа: математика
    add({"пифагор", "pythagor"},
        "Теорема Пифагора: в прямоугольном треугольнике a² + b² = c², где c — гипотенуза 📐",
        "Pythagorean theorem: a² + b² = c² in a right triangle 📐");
    add({"площад", "прямоугольник"}, "Площадь прямоугольника = a · b, площадь треугольника = a · h / 2, круга = π · r² 📏");
    add({"дискриминант", "квадратн"},
        "Для ax² + bx + c = 0: D = b² − 4ac. Если D > 0 — два корня x = (−b ± √D) / 2a, D = 0 — один, D < 0 — корней нет.");
    // русский язык
    add({"жи", "ши"}, "Правило: «жи», «ши» пиши с буквой И ✏️");
    add({"безударн", "проверочн"}, "Безударную гласную проверяй, подбирая однокоренное слово, где она под ударением: вода — воды ✏️");
    // английский
    add({"present", "simple", "настоящее"},
        "Present Simple: I work, he works. Отрицание: don't/doesn't + глагол; вопрос: Do/Does + подлежащее + глагол 🇬🇧",
        "Present Simple: I work, he works. Negative: don't/doesn't + verb; question: Do/Does + subject + verb 🇬🇧");
    add({"be", "to be", "быть"},
        "to be: I am, you are, he/she/it is, we/you/they are 🇬🇧", "to be: I am, you are, he/she/it is, we/you/they are 🇬🇧");
    // физика, химия, биология, география
    add({"ома", "закон"}, "Закон Ома: I = U / R (ток = напряжение / сопротивление) ⚡");
    add({"ньютон"}, "Второй закон Ньютона: F = m · a 🍎");
    add({"вода", "h2o"}, "Вода — H₂O: два атома водорода и один кислорода 💧");
    add({"фотосинтез"}, "Фотосинтез: растения на свету превращают CO₂ и воду в глюкозу и выделяют кислород 🌱");
    add({"москва", "столица"}, "Столица России — Москва 🏙️. Самая длинная река Европы — Волга.");
    // история
    add({"крещение", "владимир"}, "Крещение Руси — 988 год, князь Владимир Святославич 📜");
    add({"петр", "пётр"}, "Пётр I (1672–1725) — первый российский император, основал Санкт-Петербург в 1703 году 👑");
    add({"1812", "наполеон"}, "Отечественная война 1812 года: Бородинское сражение, затем отступление армии Наполеона из Москвы ⚔️");
    add({"война", "вов", "1941"}, "Великая Отечественная война: 1941–1945. День Победы — 9 мая 🎖️");
    // блогеры, игры, приложения (небольшая стартовая база — пополняй командой «запомни»)
    add({"mrbeast", "мистербист"}, "MrBeast — американский ютубер, известен масштабными челленджами и благотворительностью 🎥");
    add({"minecraft", "майнкрафт"}, "Minecraft — игра-песочница от Mojang (2011): добывай ресурсы, строй и выживай ⛏️");
    add({"roblox", "роблокс"}, "Roblox — платформа, где игроки создают игры и играют в игры других 🎮");
    add({"telegram", "телеграм"}, "Telegram — мессенджер, основатель — Павел Дуров 📱");
    add({"tiktok", "тикток"}, "TikTok — приложение с короткими видео 🎵");
    add({"блогер", "стример", "ютубер"},
        "Я знаю много блогеров и стримеров 🎥 Назови имя — расскажу, что знаю. Если не знаю, научи: "
        "«запомни: вопрос = ответ» 🦔");
}
void loadCustom() {
    ifstream f("yozhik_kb.txt"); string l;
    while (getline(f, l)) {
        auto p = l.find('|'); if (p == string::npos) continue;
        Entry e;
        for (auto& w : tokens(l.substr(0, p))) if (w.size() >= 3) e.keys.push_back(w);
        e.ru = e.en = l.substr(p + 1);
        if (!e.keys.empty()) kb.push_back(e);
    }
}
bool learn(const string& in) {
    auto p = in.find(':'); auto q = in.find('=');
    if (p == string::npos || q == string::npos || q < p) return false;
    string qs = in.substr(p + 1, q - p - 1), a = in.substr(q + 1);
    ofstream("yozhik_kb.txt", ios::app) << qs << "|" << a << "\n";
    Entry e;
    for (auto& w : tokens(qs)) if (w.size() >= 3) e.keys.push_back(w);
    e.ru = e.en = a; if (!e.keys.empty()) kb.push_back(e);
    return true;
}

// ---------- Ответы ----------
string answer(const string& in) {
    U all = dec(in);
    bool cyr = false, letters = false;
    for (char32_t c : all) { c = low(c); if (isCyr(c)) cyr = true; if (isWordCh(c)) letters = true; }
    // только смайлики
    if (!letters) return all.empty() ? "" : in + " 🦔💚";
    // калькулятор
    string r;
    if (tryCalc(in, r)) return (cyr ? "Ответ: " : "Answer: ") + r + " 🧮";
    // учим новому
    if (tokens(in).size() && tokens(in)[0] == tokens("запомни")[0] && learn(in))
        return "Запомнил! 🧠✨ Теперь я знаю больше!";
    auto tk = tokens(in);
    int best = 0; const Entry* be = nullptr;
    for (auto& e : kb) {
        int sc = 0;
        for (auto& k : e.keys) for (auto& w : tk) if (like(w, k)) { sc++; break; }
        if (sc >= best && sc > 0) { best = sc; be = &e; }
    }
    if (be) return cyr ? be->ru : be->en;
    return cyr ? "Хм, пока не знаю 🤔 Научи меня: «запомни: вопрос = ответ» — и я запомню навсегда! 🦔"
               : "Hmm, I don't know that yet 🤔 Teach me: «запомни: question = answer» 🦔";
}

int main() {
    initKB(); loadCustom();
    cout << "🦔 Ёжик: Привет! Я Ёжик. Пиши по-русски или English, считай примеры (2+2*3), "
            "спрашивай про школу, мультики, игры. Выход: «пока» или exit.\n";
    string line;
    while (true) {
        cout << "Ты: "; if (!getline(cin, line)) break;
        auto t = tokens(line);
        if (line == "exit" || line == "quit" || (!t.empty() && (t[0] == tokens("пока")[0] || t[0] == tokens("bye")[0]))) {
            cout << "🦔 Ёжик: Пока-пока! 👋\n"; break;
        }
        string a = answer(line);
        if (!a.empty()) cout << "🦔 Ёжик: " << a << "\n";
    }
}
