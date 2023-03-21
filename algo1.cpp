#include <iostream>
#include <string>
#include <queue>
#include <chrono>
#include <algorithm>
#include <regex>
#include <fstream>
// #include "CatchMemoryLeak.h"

using namespace std;

#define REGEX_CLASS_CONCRETE(name,strname,regex) \
class name : public Concrete                     \
{                                                \
public:                                          \
    name* clone() const override {               \
        return new name(*this);                  \
    }                                            \
    operator string() const override {           \
        return "<" strname ">";                  \
    }                                            \
    string toRegex() const override {            \
        return regex;                            \
    }                                            \
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
        return strname "(" "()" ")";                      \
    }                                                     \
    string toRegex() const override {                     \
        if (e==nullptr) return "(" before "()" after ")"; \
        return "(" before+e->toRegex()+after ")";         \
    }                                                     \
}

#define REGEX_CLASS_BINARY(name,strname,delim)            \
class name : public Binary                                \
{                                                         \
public:                                                   \
    using Binary::Binary;                                 \
    name* clone() const override {                        \
        return new name(*this);                           \
    }                                                     \
    operator string() const override {                    \
        string e1_string = string("");                    \
        if (e1!=nullptr)  {                               \
            e1_string = string(*e1);                      \
        }                                                 \
        string e2_string = string("");                    \
        if (e2!=nullptr)  {                               \
            e2_string = string(*e2);                      \
        }                                                 \
        return strname "("+e1_string+","+e2_string+")";   \
    }                                                     \
    string toRegex() const override {                     \
        return "("+e1->toRegex()+delim+e2->toRegex()+")"; \
    }                                                     \
}


class Regex
{
public:
    bool isConcrete;
    int closest_leaf;

    Regex() : isConcrete(false), closest_leaf(0) {}
    virtual ~Regex() = default;
    Regex(const Regex& other) = default;
    Regex& operator=(const Regex& other) = delete;
    virtual Regex* clone() const = 0;//{return new Regex(*this);}// = 0;

    virtual Regex* setClosestLeaf(Regex* token) = 0;//{return nullptr;}// = 0;
    virtual operator string() const = 0;//{return"";}// = 0;
    virtual string toRegex() const = 0;//{return"()";}// = 0;
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

    Concrete* setClosestLeaf(Regex* token) {
        return this;
    }
};

REGEX_CLASS_CONCRETE(Any,"any",".");

REGEX_CLASS_CONCRETE(Num,"num","[0-9]");

REGEX_CLASS_CONCRETE(Let,"let","[a-zA-Z]");

REGEX_CLASS_CONCRETE(Low,"low","[a-z]");

REGEX_CLASS_CONCRETE(Cap,"cap","[A-Z]");

class SpecificChar : public Concrete {
public:
    string c;

    SpecificChar(string c) : c(c) {}
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

class SpecificNum : public SpecificChar {
public:
    using SpecificChar::SpecificChar;
    SpecificNum* clone() const override {
        return new SpecificNum(*this);
    }
};

class Unary : public Regex
{
public:
    Regex* e;

    Unary(Regex* e = nullptr) : Regex() {
        if (e!=nullptr) {
            isConcrete = e->isConcrete;
            closest_leaf = e->closest_leaf + 1;
        } else {
            isConcrete = false;
            closest_leaf = 1;
        }
        Unary::e = e;
    }
    ~Unary() override {
        if (e!=nullptr) {
            delete e;
        }
    }
    Unary(const Unary& other) {
        isConcrete = other.isConcrete;
        closest_leaf = other.closest_leaf;
        if (other.e==nullptr) {
            e = nullptr;
        } else {
            e = other.e->clone();
        }
    }
    Unary& operator=(const Unary& other) = delete;

    Regex* setClosestLeaf(Regex* token) override {
        if (e==nullptr || string(*e)=="") {
            if(e!=nullptr)delete e;
            e = token;
            closest_leaf = token->closest_leaf + 1;
            isConcrete = token->isConcrete;
        } else if (!e->isConcrete) {
            e->setClosestLeaf(token);
            closest_leaf = e->closest_leaf + 1;
            isConcrete = e->isConcrete;
        }
        return this;
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
            isConcrete = false;
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
    Binary(const Binary& other) {
        Regex* old_e1 = e1;
        Regex* old_e2 = e2;
        isConcrete = other.isConcrete;
        closest_leaf = other.closest_leaf;
        if (other.e1==nullptr) {
            e1 = nullptr;
        } else {
            e1 = (other.e1)->clone();
        }
        if (other.e2==nullptr) {
            e2 = nullptr;
        } else {
            e2 = (other.e2)->clone();
        }

        if (old_e1!=nullptr) {
            // delete old_e1;
        }
        if (old_e2!=nullptr) {
            // delete old_e2;
        }
    }
    Binary& operator=(const Binary& other) = delete;

    Regex* setClosestLeaf(Regex* token) override {
        if (e1==nullptr || string(*e1)=="") {if(e1!=nullptr)delete e1;
            e1 = token;
        } else if (e2==nullptr || string(*e2)=="") {if(e2!=nullptr)delete e2;
            e2 = token;
        } else {
            if (e1->isConcrete) {
                if (e2->isConcrete) {
                    return this;
                } else {
                    e2->setClosestLeaf(token);
                }
            } else {
                if (e2->isConcrete) {
                    e1->setClosestLeaf(token);
                } else {
                    if (e1->closest_leaf<=e2->closest_leaf) {
                        e1->setClosestLeaf(token);
                    } else {
                        e2->setClosestLeaf(token);
                    }
                }
            }
        }

        if (e2==nullptr || string(*e2)=="") {
            // closest_leaf = 1;
            // isConcrete = false;
        } else {
            closest_leaf = !e1->isConcrete ? e1->closest_leaf+1 : e2->closest_leaf+1;
            closest_leaf = !e2->isConcrete ? min(closest_leaf,e2->closest_leaf+1) : closest_leaf;
            isConcrete = e1->isConcrete && e2->isConcrete;
        }
        return this;
    }
};

REGEX_CLASS_BINARY(Concat,"concat","");

REGEX_CLASS_BINARY(Or,"or","|");

REGEX_CLASS_BINARY(And,"and","&");



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

bool skip_token_for_p(const Regex* const p, const Regex* const token)
{
    // combinations with no effect or equivalent to other combinations
    if (regex_instance_of<Startwith,Endwith,Contain>(p)
         && regex_instance_of<Startwith,Endwith,Contain,Optional,Any>(token)) {
        return true;
    }

    // combinations with no effect
    if (both_regexes_instance_of<Not,Optional,Star>(p,token)) {
        return true;
    }

    //only left child can be derived
    if (both_regexes_instance_of<Concat,Or,And>(p,token)) {
        const Binary* p_binary = static_cast<const Binary*>(p);
        if (p_binary->e2==nullptr && p_binary->e1!=nullptr)
            return true;
    }

    if (regex_instance_of<Or,And>(p) && regex_instance_of<Any>(token)) {
        return true;
    }

    if (regex_instance_of<And>(p)) {
        const Binary* p_binary = static_cast<const Binary*>(p);
        if (p_binary->e1==nullptr && p_binary->e2!=nullptr) {
            if (regex_instance_of<Concrete>(p_binary->e2) && regex_instance_of<Concrete>(token)) {
                return true;
            }
        }
        if (p_binary->e1!=nullptr && p_binary->e2==nullptr) {
            if (regex_instance_of<Concrete>(p_binary->e1) && regex_instance_of<Concrete>(token)) {
                return true;
            }
        }
    }

    if (regex_instance_of<Or>(p)) {
        const Binary* p_binary = static_cast<const Binary*>(p);
        if (p_binary->e1==nullptr && p_binary->e2!=nullptr) {
            // if (both_regexes_instance_of<SpecificChar>(p_binary->e2,token)
            //     && static_cast<const Binary*>(p)) {
            //     return true;
            // }
            if (both_regexes_instance_of<Num,Let,Low,Cap,Any>(p_binary->e2,token)) {
                return true;
            }
            if (regex_instance_of<Low,Cap>(p_binary->e2) && regex_instance_of<Low,Cap>(token)) {
                return true;
            }
            if (regex_instance_of<Num>(p_binary->e2) && regex_instance_of<Num,SpecificNum>(token)
                || regex_instance_of<Num,SpecificNum>(p_binary->e2) && regex_instance_of<Num>(token)) {
                return true;
            }
        } else if (p_binary->e1!=nullptr && p_binary->e2==nullptr) {
            if (both_regexes_instance_of<Num,Let,Low,Cap,Any>(p_binary->e1,token)) {
                return true;
            }
            if (regex_instance_of<Low,Cap>(p_binary->e1) && regex_instance_of<Low,Cap>(token)) {
                return true;
            }
            if (regex_instance_of<Num>(p_binary->e1) && regex_instance_of<Num,SpecificNum>(token)
                || regex_instance_of<Num,SpecificNum>(p_binary->e1) && regex_instance_of<Num>(token)) {
                return true;
            }
        }
    }

    return false;
}

queue<Regex*> expand(Regex* p)
{
    queue<Regex*> tokens;
    tokens.push(new Num());
    tokens.push(new Let());
    tokens.push(new Low());
    tokens.push(new Cap());
    tokens.push(new Any());
    // for (int i=0; i<10; ++i) {
    //     tokens.push(new SpecificNum(to_string(i)));
    // }

    tokens.push(new Startwith());
    tokens.push(new Endwith());
    tokens.push(new Contain());
    tokens.push(new Not());
    tokens.push(new Optional());
    tokens.push(new Star());

    tokens.push(new Concat());
    tokens.push(new Or());
    tokens.push(new And());

    // tokens.push(new Or(new Num()));
    // tokens.push(new Or(new Num(),new Let()));
    if (p==nullptr) {
        delete p;
        return tokens;
    }
    queue<Regex*> children;
    while (tokens.size()>0) {
        Regex* token = tokens.front();
        tokens.pop();
        if (skip_token_for_p(p,token)) {
            delete token;
            continue;
        }
        Regex* q = p->clone();
        q->setClosestLeaf(token);
        children.push(q);
    }
    delete p;
    return children;
}

Regex* algo1(vector<string> accept, vector<string> reject)
{
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();

    queue<Regex*> worklist;
    worklist.push(nullptr);
    while (worklist.size() > 0) {
        Regex* p = worklist.front();
        worklist.pop();
// if(p!=nullptr){cout<<"p: "<<string(*p)<<" isConcrete: "<<(p->isConcrete ? "True" : "False")<<" closest_leaf: "<<p->closest_leaf<<endl;}
        if (p!=nullptr && p->isConcrete) {
            cout<<"p: "+string(*p)<<endl;
            cout<<chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now()-begin).count()/1000000.0<<":";
            cout<<"\t\t"+p->toRegex()<<endl;
            regex r(p->toRegex());
            bool acc = true;
            for (int i=0; i<accept.size();++i) {
                if (!regex_match(accept[i],r)) {
                    acc = false;
                    break;
                }
            }
            if (acc) {
                bool rej = true;
                for (int i=0; i<reject.size();++i) {
                    if (regex_match(accept[i],r)) {
                        rej = false;
                        break;
                    }
                }
                if (rej) {
                //     cout<<string(*p)<<" matches"<<endl;
                //     cout<<chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now()-begin).count()/1000000.0<<":";
                //     cout<<"\t\t"+p->toRegex()<<endl;
                    while (worklist.size() > 0) {
                        Regex* p_ = worklist.front();
                        worklist.pop();
                        delete p_;
                    }
                    return p;
                }
            }
            delete p;
        } else {
            queue<Regex*> worklist_ = expand(p);
            while (worklist_.size() > 0) {
                worklist.push(worklist_.front());
                worklist_.pop();
            }
        }
    }
    return nullptr;
}

#undef REGEX_CLASS_CONCRETE
#undef REGEX_CLASS_UNARY
#undef REGEX_CLASS_BINARY


int main()
{
    string e("123");
    regex r("[0-9]*");
    regex_match(e,r);

    vector<string> accept = {"80","81","82","83","84"};
    vector<string> reject;// = {"85","86","87","88","89"};

    // vector<string> exact;
    // vector<string> unmatch;
    // vector<string> general;

    // vector<string> include;
    // vector<string> exclude;

    Regex* p = algo1(accept,reject);
    if (p!=nullptr) {
        delete p;
    } else {
        cout<<"the synthesizer didn't find any regular expressoin"<<endl;
    }
    // reportUnreleasedHeap();
    return 0;
}






