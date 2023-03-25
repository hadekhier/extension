#include <iostream>
#include <string>
#include <queue>
#include <chrono>
#include <algorithm>
#include <regex>
#include <vector>

// #include "CatchMemoryLeak.h"
// #include "dbg.h"

using namespace std;

const string WHITESPACE = " \n\r\t\f\v";

string ltrim(const string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return start==string::npos ? "" : s.substr(start);
}
string rtrim(const string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return end==string::npos ? "" : s.substr(0,end+1);
}
string trim(const string& s)
{
    return ltrim(rtrim(s));
}

string remove_duplicates(const string& str)
{
    string res("");
    for (auto c = str.cbegin(); c!=str.cend(); ++c) {
        bool add = true;
        for (auto old = res.cbegin(); old!=res.cend(); ++old) {
            if (*old == *c) {
                add = false;
                break;
            }
        }
        if (add) res.push_back(*c);
    }

    return res;
}

#define REGEX_CLASS_CONCRETE(name,strname,regex,is_what) \
class name : public Concrete                             \
{                                                        \
public:                                                  \
    name() : Concrete() {is_what = true;}                \
    name* clone() const override {                       \
        return new name(*this);                          \
    }                                                    \
    operator string() const override {                   \
        return "<" strname ">";                          \
    }                                                    \
    string toRegex() const override {                    \
        return regex;                                    \
    }                                                    \
}

#define REGEX_CLASS_UNARY(name,strname,before,after)      \
class name : public Unary                                 \
{                                                         \
public:                                                   \
    using Unary::Unary;                                   \
    name* clone() const override {                        \
        return new name(*this);                           \
    }                                                     \
    operator string() const override {                    \
        if (e!=nullptr)                                   \
            return strname "("+string(*e)+")";            \
        return strname "(?)";                             \
    }                                                     \
    string toRegex() const override {                     \
        if (e==nullptr) return "(" before "()" after ")"; \
        return "(" before+e->toRegex()+after ")";         \
    }                                                     \
}

#define REGEX_CLASS_BINARY(name,strname,delim)          \
class name : public Binary                              \
{                                                       \
public:                                                 \
    using Binary::Binary;                               \
    name* clone() const override {                      \
        return new name(*this);                         \
    }                                                   \
    operator string() const override {                  \
        string e1_string = string("?");                 \
        if (e1!=nullptr)  {                             \
            e1_string = string(*e1);                    \
        }                                               \
        string e2_string = string("?");                 \
        if (e2!=nullptr)  {                             \
            e2_string = string(*e2);                    \
        }                                               \
        return strname "("+e1_string+","+e2_string+")"; \
    }                                                   \
    string toRegex() const override {                   \
        string e1_regex = string("()");                 \
        if (e1!=nullptr)  {                             \
            e1_regex = e1->toRegex();                   \
        }                                               \
        string e2_regex = string("()");                 \
        if (e2!=nullptr)  {                             \
            e2_regex = e2->toRegex();                   \
        }                                               \
        return "("+e1_regex+delim+e2_regex+")";         \
    }                                                   \
}

#define REGEX_CLASS_SPECIFIC(name,is_what)             \
class name : public SpecificChar                       \
{                                                      \
public:                                                \
    name(string c) : SpecificChar(c) {is_what = true;} \
    name(char c) : SpecificChar(c) {is_what = true;}   \
    name* clone() const override {                     \
        return new name(*this);                        \
    }                                                  \
}


class Regex
{
public:
    bool isConcrete;
    int closest_leaf;
    bool isNum;
    bool isLet;
    bool isLow;
    bool isCap;

    Regex() : isConcrete(false), closest_leaf(0), isNum(false), isLet(false), isLow(false), isCap(false) {}
    virtual ~Regex() = default;
    Regex(const Regex& other) = default;
    Regex& operator=(const Regex& other) = delete;
    virtual Regex* clone() const = 0;//{return new Regex(*this);}// = 0;

    virtual bool setClosestLeaf(Regex* token) = 0;//{return nullptr;}// = 0;
    virtual operator string() const = 0;//{return"";}// = 0;
    virtual string toRegex() const = 0;//{return"()";}// = 0;
    virtual const Regex* getParentOfNextToken() const = 0;//{return this;}
};


class Concrete : public Regex
{
public:
    Concrete() : Regex() {
        isConcrete = true;
    }
    ~Concrete() override = default;
    Concrete(const Concrete& other) = default;
    Concrete& operator=(const Concrete& other) = delete;

    bool setClosestLeaf(Regex* token) override {
        return false;
    }
    const Regex* getParentOfNextToken() const override {
        return this;
    }
};

REGEX_CLASS_CONCRETE(Any,"any",".",isConcrete);//no special indecator
REGEX_CLASS_CONCRETE(Num,"num","[0-9]",isNum);
REGEX_CLASS_CONCRETE(Let,"let","[a-zA-Z]",isLet);
REGEX_CLASS_CONCRETE(Low,"low","[a-z]",isLow);
REGEX_CLASS_CONCRETE(Cap,"cap","[A-Z]",isCap);

class SpecificChar : public Concrete {
public:
    string c;
// string name;

    SpecificChar(string c) : c(c) {}//{name="CHAR";}
    SpecificChar(char c) : c(string(1,c)) {}//{name="CHAR";}
    virtual SpecificChar* clone() const override {
        return new SpecificChar(*this);
    }
    operator string() const override {
        return "<"+c+">";
    }
    string toRegex() const override {
        return c;
    }
};

REGEX_CLASS_SPECIFIC(SpecificNum,isNum);
REGEX_CLASS_SPECIFIC(SpecificLet,isLet);
REGEX_CLASS_SPECIFIC(SpecificLow,isLow);
REGEX_CLASS_SPECIFIC(SpecificCap,isCap);
//SpSeq (big concat)
//big or

SpecificChar* createSpecificChar(char c)
{
    if ('0'<=c && c<='9') return new SpecificNum(c);
    else if ('a'<=c && c<='z') return new SpecificLow(c);
    else if ('A'<=c && c<='Z') return new SpecificCap(c);
    else return new SpecificChar(c);
}

class Unary : public Regex
{
public:
    Regex* e;

    Unary(Regex* e = nullptr) : Regex() {
        if (e!=nullptr) {
            isConcrete = e->isConcrete;
            closest_leaf = e->closest_leaf + 1;
        } else {
            // isConcrete = false;
            closest_leaf = 1;
        }
        Unary::e = e;
    }
    ~Unary() override {
        if (e!=nullptr) {
            delete e;
        }
    }
    Unary(const Unary& other) : Regex(other) {
        e = other.e==nullptr ? nullptr : other.e->clone();
    }
    Unary& operator=(const Unary& other) = delete;

    bool setClosestLeaf(Regex* token) override {
        bool ischanged = false;
        if (e==nullptr) {
            e = token;
            closest_leaf = token->closest_leaf + 1;
            isConcrete = token->isConcrete;
            ischanged = true;
        } else if (!e->isConcrete) {
            ischanged = e->setClosestLeaf(token);
            closest_leaf = e->closest_leaf + 1;
            isConcrete = e->isConcrete;
        }
        return ischanged;
    }
    const Regex* getParentOfNextToken() const override {
        if (e==nullptr) {
            return this;
        }
        return e->getParentOfNextToken();
    }
};

REGEX_CLASS_UNARY(Startwith,"startwith","",".?*");
REGEX_CLASS_UNARY(Endwith,"endwith",".?*","");
REGEX_CLASS_UNARY(Contain,"contain",".?*",".?*");
REGEX_CLASS_UNARY(Not,"not","NOT(",")"); ///___________________________HOW_IN_REGEX____________________________
REGEX_CLASS_UNARY(Optional,"optional","","?");
REGEX_CLASS_UNARY(Star,"star","","*");


class Binary : public Regex
{
public:
    Regex* e1;
    Regex* e2;

    Binary(Regex* e1 = nullptr, Regex* e2 = nullptr) : Regex() {
        if (e1!=nullptr) {
            isConcrete = e1->isConcrete;
            closest_leaf = e1->closest_leaf + 1;
        } else {
            // isConcrete = false;
            closest_leaf = 1;
        }
        if (e2!=nullptr) {
            isConcrete = isConcrete && e2->isConcrete;
            closest_leaf = min(closest_leaf,e2->closest_leaf+1);
        } else {
            isConcrete = false;
            closest_leaf = 1;
        }
        Binary::e1 = e1;
        Binary::e2 = e2;
    }
    ~Binary() override {
        if (e1!=nullptr) {
            delete e1;
        }
        if (e2!=nullptr) {
            delete e2;
        }
    }
    Binary(const Binary& other) : Regex(other) {
        e1 = other.e1==nullptr ? nullptr : other.e1->clone();
        e2 = other.e2==nullptr ? nullptr : other.e2->clone();
    }
    Binary& operator=(const Binary& other) = delete;

    virtual bool setClosestLeaf(Regex* token) override {
        bool ischanged = false;
        if (e1==nullptr) {
            e1 = token;
            ischanged = true;
        } else if (e2==nullptr) {
            e2 = token;
            ischanged = true;
        } else {
            if (e1->isConcrete) {
                if (e2->isConcrete) {
                    return false;
                } else {
                    ischanged = e2->setClosestLeaf(token);
                }
            } else {
                if (e2->isConcrete) {
                    ischanged = e1->setClosestLeaf(token);
                } else {
                    if (e1->closest_leaf<=e2->closest_leaf) {
                        ischanged = e1->setClosestLeaf(token);
                    } else {
                        ischanged = e2->setClosestLeaf(token);
                    }
                }
            }
        }

        if (e2==nullptr) {
            // isConcrete = false;
            // closest_leaf = 1;
        } else {
            isConcrete = e1->isConcrete && e2->isConcrete;
            closest_leaf = !e1->isConcrete ? e1->closest_leaf+1 : e2->closest_leaf+1; //temporary value
            closest_leaf = !e2->isConcrete ? min(closest_leaf,e2->closest_leaf+1) : closest_leaf; //final value
        }

        return ischanged;
    }
    const Regex* getParentOfNextToken() const override {
        if (e1==nullptr) {
            return this;
        } else if (e2==nullptr) {
            return this;
        } else {
            if (e1->isConcrete) {
                if (e2->isConcrete) {
                    return this;
                } else {
                    return e2->getParentOfNextToken();
                }
            } else {
                if (e2->isConcrete) {
                    return e1->getParentOfNextToken();
                } else {
                    if (e1->closest_leaf<=e2->closest_leaf) {
                        return e1->getParentOfNextToken();
                    } else {
                        return e2->getParentOfNextToken();
                    }
                }
            }
        }
    }
};

REGEX_CLASS_BINARY(Concat,"concat","");
REGEX_CLASS_BINARY(And,"and","&");

class Or : public Binary
{
public:
    Or(Regex* e1 = nullptr, Regex* e2 = nullptr) : Binary(e1,e2) {
        if (e1!=nullptr) {
            isNum = e1->isNum;
            isLow = e1->isLow;
            isCap = e1->isCap;
        } else {
            // isNum = false;
            // isLow = false;
            // isCap = false;
        }
        if (e2!=nullptr) {
            isNum = isNum && e2->isNum;
            isLow = isLow && e2->isLow;
            isCap = isCap && e2->isCap;
        } else {
            isNum = false;
            isLow = false;
            isCap = false;
        }
    }
    bool setClosestLeaf(Regex* token) override {
        if (Binary::setClosestLeaf(token)==false) {
            return false;
        }

        if (e2==nullptr) {
            // isNum = false;
            // isLow = false;
            // isCap = false;
        } else {
            isNum = e1->isNum && e2->isNum;
            isLow = e1->isLow && e2->isLow;
            isCap = e1->isCap && e2->isCap;
        }

        return true;
    }

    Or *clone() const override {
        return new Or(*this);
    }
    operator string() const override {
        string e1_string = string("?");
        if (e1 != nullptr) {
            e1_string = string(*e1);
        }
        string e2_string = string("?");
        if (e2 != nullptr) {
            e2_string = string(*e2);
        }
        return "or("+e1_string+","+e2_string+")";
    }
    string toRegex() const override {
        string e1_regex = string("()");
        if (e1 != nullptr) {
            e1_regex = e1->toRegex();
        }
        string e2_regex = string("()");
        if (e2 != nullptr) {
            e2_regex = e2->toRegex();
        }
        return "("+e1_regex+"|"+e2_regex+")";
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T_1>
inline bool regex_instance_of(const Regex* const p)
{
    return  dynamic_cast<const T_1*>(p)!=nullptr;
}
template <typename T_1, typename T_2, typename ...T_rest>
inline bool regex_instance_of(const Regex* const p)
{
    return regex_instance_of<T_1>(p) || regex_instance_of<T_2,T_rest...>(p);
}

template <typename T_1>
inline bool both_regexes_instance_of(const Regex* const p, const Regex* const token)
{
    return  regex_instance_of<T_1>(p) && regex_instance_of<T_1>(token);
}
template <typename T_1, typename T_2, typename ...T_rest>
inline bool both_regexes_instance_of(const Regex* const p, const Regex* const token)
{
    return both_regexes_instance_of<T_1>(p,token) || both_regexes_instance_of<T_2,T_rest...>(p,token);
}

Regex* stringToRegex(const string& str)//str is valid
{
    const string str_stripped = trim(str);

    //a
    //a( )
    // a( , )
    string root;
    Regex* e1 = nullptr;
    Regex* e2 = nullptr;

    int rparen_pos = str_stripped.find('(');
    if ((size_t)rparen_pos == string::npos) {//concrete
        root = str_stripped;
    } else {
        root = string(str_stripped.cbegin(),str_stripped.cbegin() + rparen_pos);
        auto iter = str_stripped.cbegin() + rparen_pos+1;
        int parens = 0;
        while(iter!=str_stripped.cend()) {
            if (parens==0 && *iter==',') break;
            else if (*iter=='(') ++parens;
            else if (*iter==')') --parens;
            ++iter;
        }
        if (iter==str_stripped.cend()) {//Unary
            e1 = stringToRegex(string(str_stripped.cbegin() + rparen_pos+1,str_stripped.cbegin()+str_stripped.size()-1));
        } else {//binary
            e1 = stringToRegex(string(str_stripped.cbegin() + rparen_pos+1,iter));
            e2 = stringToRegex(string(iter+1,str_stripped.cbegin()+str_stripped.size()-1));
        }
    }

    if (root.size()==1) return createSpecificChar(root[0]);
    else if (root=="num") return new Num();
    else if (root=="let") return new Let();
    else if (root=="low") return new Low();
    else if (root=="cap") return new Cap();
    else if (root=="any") return new Any();

    else if (root=="startwith") return new Startwith(e1);
    else if (root=="endwith") return new Endwith(e1);
    else if (root=="contain") return new Contain(e1);
    // else if (root=="not") return new Not(e1);
    else if (root=="optional") return new Optional(e1);
    else if (root=="star") return new Star(e1);

    else if (root=="concat") return new Concat(e1,e2);
    else if (root=="or") return new Or(e1,e2);
    else if (root=="and") return new And(e1,e2);

    return nullptr;
}

template<typename T>
inline void token_push_concrete_if_needed(queue<Regex*>& tokens,
    const vector<Regex*>& literal, const vector<Regex*>& general,
    const vector<Regex*>& include, const vector<Regex*>& exclude_node)
{
    for (auto iter = literal.cbegin(); iter!=literal.cend(); ++iter) {
        if (regex_instance_of<T>(*iter)) {
            return;
        }
    }
    for (auto iter = general.cbegin(); iter!=general.cend(); ++iter) {
        if (regex_instance_of<T>(*iter)) {
            return;
        }
    }
    for (auto iter = include.cbegin(); iter!=include.cend(); ++iter) {
        if (regex_instance_of<T>(*iter)) {
            return;
        }
    }
    for (auto iter = exclude_node.cbegin(); iter!=exclude_node.cend(); ++iter) {
        if (regex_instance_of<T>(*iter)) {
            return;
        }
    }

    T* ptr = new T();
cout<<string(*ptr)<<" ";
    tokens.push(ptr);
}
template<typename T>
inline void token_push_unary_if_needed(queue<Regex*>& tokens,
    const vector<Regex*>& literal, const vector<Regex*>& general,
    const vector<Regex*>& include, const vector<Regex*>& exclude_node)
{
    for (auto iter = include.cbegin(); iter!=include.cend(); ++iter) {
        if (regex_instance_of<T>(*iter) && static_cast<Unary*>(*iter)->e==nullptr) {
            return;
        }
    }
    for (auto iter = exclude_node.cbegin(); iter!=exclude_node.cend(); ++iter) {
        if (regex_instance_of<T>(*iter) && static_cast<Unary*>(*iter)->e==nullptr) {
            return;
        }
    }

    T* ptr = new T();
cout<<string(*ptr)<<" ";
    tokens.push(ptr);
}
template<typename T>
inline void token_push_binary_if_needed(queue<Regex*>& tokens,
    const vector<Regex*>& literal, const vector<Regex*>& general,
    const vector<Regex*>& include, const vector<Regex*>& exclude_node)
{
    for (auto iter = include.cbegin(); iter!=include.cend(); ++iter) {
        if (regex_instance_of<T>(*iter) && static_cast<Binary*>(*iter)->e1==nullptr && static_cast<Binary*>(*iter)->e2==nullptr) {
            return;
        }
    }
    for (auto iter = exclude_node.cbegin(); iter!=exclude_node.cend(); ++iter) {
        if (regex_instance_of<T>(*iter) && static_cast<Binary*>(*iter)->e1==nullptr && static_cast<Binary*>(*iter)->e2==nullptr) {
            return;
        }
    }

    T* ptr = new T();
cout<<string(*ptr)<<" ";
    tokens.push(ptr);
}

//might take the ptrs and not the clones bc they are unused afterwards
//suppose exclude node doesnt conflict with the includes
//if char was set general, dont add it to specificChars => get index of chosen chars / git number of chars set general (88 has 2 chars "8" so we need both to be selected to not use "8")
queue<Regex*> set_tokens(const vector<string>& accept_examples,
    const vector<Regex*>& literal, const vector<Regex*>& general,
    const vector<Regex*>& include, const vector<Regex*>& exclude_node)
{
    queue<Regex*> tokens;
cout<<"the tokens: ";
    for (auto iter = literal.cbegin(); iter!=literal.cend(); ++iter) {
cout<<string(**iter)<<" ";
        tokens.push((*iter)->clone());
    }
    for (auto iter = general.cbegin(); iter!=general.cend(); ++iter) {
cout<<string(**iter)<<" ";
        tokens.push((*iter)->clone());
    }
    for (auto iter = include.cbegin(); iter!=include.cend(); ++iter) {
cout<<string(**iter)<<" ";
        tokens.push((*iter)->clone());
    }

    token_push_concrete_if_needed<Num>(tokens,literal,general,include,exclude_node);
    token_push_concrete_if_needed<Let>(tokens,literal,general,include,exclude_node);
    token_push_concrete_if_needed<Low>(tokens,literal,general,include,exclude_node);
    token_push_concrete_if_needed<Cap>(tokens,literal,general,include,exclude_node);
    token_push_concrete_if_needed<Any>(tokens,literal,general,include,exclude_node);

    //accept \ exclude_nodes -> specificChars (maybe specificNums SpLets SpCaps SpLows. for Or optimizing)
    for (auto example_iter = accept_examples.cbegin(); example_iter!=accept_examples.cend(); ++example_iter) {
        for (auto char_iter = example_iter->cbegin(); char_iter!=example_iter->cend(); ++char_iter) {
            bool push = true;
            for (auto token_iter = literal.cbegin(); token_iter!=literal.cend(); ++token_iter) {
                if ((*token_iter)->toRegex() == string(1,*char_iter)) {
                    push = false;
                    break;
                }
            }
            if (!push) continue;

            for (auto token_iter = include.cbegin(); token_iter!=include.cend(); ++token_iter) {
                if ((*token_iter)->toRegex() == string(1,*char_iter)) {
                    push = false;
                    break;
                }
            }
            if (!push) continue;

            for (auto token_iter = exclude_node.cbegin(); token_iter!=exclude_node.cend(); ++token_iter) {
                if ((*token_iter)->toRegex() == string(1,*char_iter)) {//TODO change to is<specificChar> and compare toRegex[0] (if specificSeq and bigOr dont inhirit it)
                    push = false;
                    break;
                }
            }
            if (!push) continue;

cout<<"<"<<*char_iter<<"> ";
            tokens.push(createSpecificChar(*char_iter));
        }
    }

    token_push_unary_if_needed<Startwith>(tokens,literal,general,include,exclude_node);
    token_push_unary_if_needed<Endwith>(tokens,literal,general,include,exclude_node);
    token_push_unary_if_needed<Contain>(tokens,literal,general,include,exclude_node);
    // token_push_unary_if_needed<Not>(tokens,literal,general,include,exclude_node);
    token_push_unary_if_needed<Optional>(tokens,literal,general,include,exclude_node);
    token_push_unary_if_needed<Star>(tokens,literal,general,include,exclude_node);

    token_push_binary_if_needed<Concat>(tokens,literal,general,include,exclude_node);
    token_push_binary_if_needed<Or>(tokens,literal,general,include,exclude_node);
    token_push_binary_if_needed<And>(tokens,literal,general,include,exclude_node);

cout<<"\n";
    return tokens;
}

//is it useful to have parent's type pointing to child1 and child2
bool subtree_is_useless(const Regex* const parent, const Regex* const child1, const Regex* const child2 = nullptr)
{
    if (regex_instance_of<Startwith,Endwith,Contain>(parent)
         && regex_instance_of<Startwith,Endwith,Contain,Optional,Any>(child1)) {
        return true;
    }

    // combinations with no effect
    if (both_regexes_instance_of<Not,Optional,Star>(parent,child1)) return true;

    //only left child can be derived //use might ruit it with {include or(<num>,?) exclude <num>}
    if (both_regexes_instance_of<Concat,Or,And>(parent,child2)) return true;
        // const Binary* p_binary = static_cast<const Binary*>(parent);

    if (regex_instance_of<Or,And>(parent)
            && (regex_instance_of<Any>(child1) ||  regex_instance_of<Any>(child2))) {
        return true;
    }

    if (regex_instance_of<And>(parent)
            && (regex_instance_of<Concrete>(child1) && regex_instance_of<Concrete>(child2))) {
        return true;
    }

    if (regex_instance_of<Or>(parent)) {
        // {
        //     Regex* q=p->clone();
        //     q->setClosestLeaf(token->clone());
        //     if (q->toRegex()=="((0|1)|[0-9])") {
        //         cout<<"HERE\n";
        //     }
        //     delete q;
        // }
        if (both_regexes_instance_of<Concrete>(child1,child2)) {
            if (both_regexes_instance_of<SpecificChar>(child1,child2)
                    && static_cast<const SpecificChar*>(child1)->c>=static_cast<const SpecificChar*>(child2)->c) {
                return true;
            }
            if (both_regexes_instance_of<Num,Let,Low,Cap,Any>(child1,child2)) return true;
            if (regex_instance_of<Low,Cap>(child1) && regex_instance_of<Low,Cap>(child2)) return true;
        }
        if ((regex_instance_of<Num>(child1) && child2->isNum)
                || (child1->isNum && regex_instance_of<Num>(child2))) {
            return true;
        }
        if (child1->toRegex()==child2->toRegex()) return true;
    }

    return false;
}

//send skip_token_for_p to the path token will go to and return that
bool skip_token_for_p(const Regex* const p, const Regex* const token)
{
    //or(start(a),start(b)) = start(or(a,b)))
    //and(start(a),start(b)) = start(and(a,b)))
    //or(end(a),end(b)) = end(or(a,b)))
    //and(end(a),end(b)) = end(and(a,b)))

    // combinations with no effect or equivalent to other combinations

    const Regex* parent = p->getParentOfNextToken(); // parent => at least one son is nullptr

    if (regex_instance_of<Unary>(parent)) {
        return subtree_is_useless(parent,token);
    }

    if (regex_instance_of<Binary>(parent)) {
        const Binary* p_binary = static_cast<const Binary*>(parent);
        if (p_binary->e1!=nullptr || p_binary->e2!=nullptr) {
            const Regex* child1 = p_binary->e1;
            const Regex* child2 = p_binary->e2;
            if (p_binary->e1!=nullptr) {
                child2 = token;
            } else {
                child1 = token;
            }

            return subtree_is_useless(parent,child1,child2);
        }
    }

    return false; //send skip_token_for_p to the path token will go to and return that TODO TOOOOOOOOOOOOOOOOOOOOOODOOOOOOOOOOOOOOOOOOOOO
}

//use exclude
queue<Regex*> expand(const queue<Regex*>& tokens, Regex* p, const vector<Regex*>& exclude_tree, queue<Regex*>& worklist_concrete)
{
    queue<Regex*> tokens_copy(tokens);

    if (p==nullptr) {
        queue<Regex*> tokens_clone;
        while (tokens_copy.size()>0) {
            Regex* token = tokens_copy.front()->clone();
            tokens_copy.pop();
            if (token->isConcrete) worklist_concrete.push(token);
            else tokens_clone.push(token);
        }
        return tokens_clone;
    }

    queue<Regex*> children_partial;
    while (tokens_copy.size()>0) {
        Regex* token = tokens_copy.front()->clone();
// cout<<"token: "<<string(*token)<<endl;
        tokens_copy.pop();
// cout<<"skip_token_for_p(p,token): "<<string(*p)<<' '<<string(*token);
        bool skip = skip_token_for_p(p,token);
        if (skip) {
// cout<<" skip"<<endl;
            delete token;
            continue;
        }
// cout<<endl;
        Regex* q = p->clone();
        q->setClosestLeaf(token);
        bool push = true;
        for (auto iter = exclude_tree.cbegin(); iter!=exclude_tree.cend(); ++iter) {
            if (q->toRegex().find((*iter)->toRegex()) != std::string::npos) {
                push = false;
                delete q;
                break;
            }
        }
        if (push) {
// cout<<"q: "<<string(*q)<<endl<<" isNum:"<<q->isNum<<" isLet:"<<q->isLet<<" isCap:"<<q->isCap<<" isLow:"<<q->isLow<<endl;
            if (q->isConcrete) worklist_concrete.push(q);
            else children_partial.push(q);
        }
    }

    delete p;
    return children_partial;
}

//sort
Regex* algo1(const queue<Regex*>& tokens, const vector<string>& accept,
    const vector<string>& reject, const vector<Regex*>& exclude_tree)
{
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();

    queue<Regex*> worklist;
    queue<Regex*> worklist_concrete;
    worklist.push(nullptr);
    while (worklist.size()>0 || worklist_concrete.size()>0) {
        while (worklist_concrete.size()>0) {
            Regex* p = worklist_concrete.front();
            worklist_concrete.pop();
            cout<<"p: "+string(*p)<<endl;
            cout<<chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now()-begin).count()/1000000.0<<":";
            cout<<"\t\t"+p->toRegex()<<endl;
            std::regex r(p->toRegex());
            bool acc = true;
            for (int i=0; (size_t)i<accept.size();++i) {
                if (!regex_match(accept[i],r)) {
                    acc = false;
                    break;
                }
            }
            if (acc) {
                bool rej = true;
                for (int i=0; (size_t)i<reject.size();++i) {
                    if (regex_match(reject[i],r)) {
                        rej = false;
                        break;
                    }
                }
                if (rej) {
                    cout<<string(*p)<<" matches"<<endl;
                    cout<<chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now()-begin).count()/1000000.0<<":";
                    cout<<"\t\t"+p->toRegex()<<endl;
                    while (worklist_concrete.size() > 0) {
                        Regex* p_ = worklist_concrete.front();
                        worklist_concrete.pop();
                        delete p_;
                    }
                    while (worklist.size() > 0) {
                        Regex* p_ = worklist.front();
                        worklist.pop();
                        delete p_;
                    }
                    return p;
                }
            }
            delete p;
        }

        //sort worklist
        Regex* p = worklist.front();
        worklist.pop();
// if(p!=nullptr){cout<<"p: "<<string(*p)<<endl;}//" isConcrete: "<<(p->isConcrete ? "True" : "False")<<" closest_leaf: "<<p->closest_leaf<<endl;}
        //TODO: pass worklist with ref to do less copies
        queue<Regex*> worklist_ = expand(tokens,p,exclude_tree,worklist_concrete);
        while (worklist_.size() > 0) {
            worklist.push(worklist_.front());
            worklist_.pop();
        }
    }
    return nullptr;
}

#undef REGEX_CLASS_CONCRETE
#undef REGEX_CLASS_UNARY
#undef REGEX_CLASS_BINARY
#undef REGEX_CLASS_SPECIFIC

//button show example
//feature: save session
//if chosen LIT then GEN?
int main()//not abc = [^a]*[^b]*[^c*] ??
{
    // string r1("or(8,8)");
    // Regex* R1 = stringToRegex(r1);
    // const Binary* R1_binary = static_cast<const Binary*>(R1);
    // cout<<R1<<"\n"<<R1_binary<<"\n";
    // cout <<both_regexes_instance_of<SpecificChar>(R1_binary->e1,R1_binary->e2);
    // cout <<static_cast<const SpecificChar*>(R1_binary->e1)->toRegex();
    // cout <<static_cast<const SpecificChar*>(R1_binary->e2)->toRegex();
    // cout <<(static_cast<const SpecificChar*>(R1_binary->e1)->toRegex()>=static_cast<const SpecificChar*>(R1_binary->e2)->toRegex());


    // string r2("endwith(startwith(<3>))");
    // Regex* R2 = stringToRegex(r2);
    // const Unary* R2_unary = static_cast<const Unary*>(R2);
    // cout<<R2<<"\n"<<R2_unary<<"\n";
    // cout <<regex_instance_of<Startwith,Endwith,Contain>(R2_unary);
    // cout <<regex_instance_of<Startwith,Endwith,Contain>(R2_unary->e);

    // string r("or(<num>,<2>)");

    // make it end
    // queue<Regex*> tokens;
    // tokens.push(new Or());
    // tokens.push(new Num());
    // tokens.push(new SpecificNum("2"));
// return 0;

    //collect all strings as sets
    //for each string add the Regex() for it
    //tell user if exclude has include/literal/general
    //exclude can have complicated structure, not only one node
    //clean the input

    //in worklist get concretes to front
    //in expand set hight limit to expand with concretes only, and after that try Unary Binary (dont create untill finish of prev height)

    //from whatever is not in acc, like letters now, remove from tokens
    vector<string> accept_examples = {"80","81","82","83","84"};//if max len is t, then concat must use optional after t leaves
    vector<string> reject_examples = {"85","86","87","88","89"};

    vector<string> literal_str = {"80","81","2","3","4"};
    vector<string> general_str = {"num"};

    vector<string> include_Str = {"or","concat"};
    vector<string> exclude_Str = {"and","star","optional","contain","let","low","cap"};

    //no duplicates
    //all strings stripped and valid
    //sort each str set and (nlogn) find if exclude has something common with any of the other (n)

    //only seperate letters!!! using concat'ed needs changes ???????????
    vector<Regex*> literal;
    for (auto iter = literal_str.begin(); iter!=literal_str.end(); ++iter) {
        if (iter->size() > 1) {
            //cat_over_each_examples_single_chars
            Concat* concat_ptr = new Concat();
            for (auto c = (*iter).begin(); c!=(*iter).end(); ++c) {
                if (concat_ptr->e2 == nullptr) {
                    concat_ptr->setClosestLeaf(createSpecificChar(*c));
                } else {
                    Concat* new_root = new Concat(concat_ptr,createSpecificChar(*c));
                    concat_ptr = new_root;
                }
            }
            literal.push_back(concat_ptr);
        }
    }

    string all_literals("");
    for (auto iter = literal_str.begin(); iter!=literal_str.end(); ++iter) {
        all_literals+=*iter;
    }
    all_literals = remove_duplicates(all_literals);
    std::sort(all_literals.begin(), all_literals.end());

    for (auto c = all_literals.begin(); c!=all_literals.end(); ++c) {
        literal.push_back(createSpecificChar(*c));
    }

    //or_over_all_the_single_chars
    if (all_literals.size()>1) {
        Or* or_ptr = new Or();
        for (auto c = all_literals.begin(); c!=all_literals.end(); ++c) {
            if (or_ptr->e1 == nullptr || or_ptr->e2 == nullptr) {
                or_ptr->setClosestLeaf(createSpecificChar(*c));
            } else {
                Or* new_root = new Or(or_ptr,createSpecificChar(*c));
                or_ptr = new_root;
            }
        }

        literal.push_back(or_ptr);
    }

    vector<Regex*> general;
    for (auto iter = general_str.begin(); iter!=general_str.end(); ++iter) {
        general.push_back(stringToRegex(*iter));
    }

    vector<Regex*> include;
    for (auto iter = include_Str.begin(); iter!=include_Str.end(); ++iter) {
        include.push_back(stringToRegex(*iter));
    }

    vector<Regex*> exclude_node;
    vector<Regex*> exclude_tree;
    for (auto iter = exclude_Str.begin(); iter!=exclude_Str.end(); ++iter) {
        Regex* ex = stringToRegex(*iter);
        if (regex_instance_of<Unary>(ex) && static_cast<Unary*>(ex)->e!=nullptr) {
            exclude_tree.push_back(ex);
        } else if (regex_instance_of<Binary>(ex)
            && (static_cast<Binary*>(ex)->e1!=nullptr || static_cast<Binary*>(ex)->e2!=nullptr)) {
            exclude_tree.push_back(ex);
        } else {
            exclude_node.push_back(ex);
        }
    }

    queue<Regex*> tokens = set_tokens(accept_examples,literal,general,include,exclude_node);
    // queue<Regex*> tokens;
    // tokens.push(new Or());
    // tokens.push(new Num());
    // tokens.push(new SpecificNum("2"));
// return 0;
    Regex* p = algo1(tokens,accept_examples,reject_examples,exclude_tree);
    if (p!=nullptr) {
        delete p;
    } else {
        cout<<"the synthesizer didn't find any regular expressoin"<<endl;
    }
    // reportUnreleasedHeap();
    return 0;
}

//TODO feasable
//TODO skip_token_if_p (bug and improve)
//TODO tuple and move and ref and faster containers
//TOODO//do e1@8@num so you dont take 8 from example 1 in the SpecifiChars//then to get 8[0-9] we need 88 literal-general
//TODO use specificchar to concat all chars instead of concatting tree
//TODO create BIG_OR with string chars whre toRegex adds "|" between each 2 chars
//TODO or(or(),) and or() will ceate duplicates



