

#include <vector>
#include <set>
#include <map>
#include <map>
#include <iostream>
#include <random>
#include <cassert>
#include <string>
#include <algorithm>
#include <chrono>

#define POSTCONDITION(c) assert(c)

using namespace std;


struct Scheduler
{
    using Student=int;
    using Course=char;
    using Room=int;
    using Timeslice=int;
    using Preferences = map<Student,set<Course> >;
    using Reservation = pair<Room,Timeslice>;
    using Schedule = map<Reservation,Course>;

    Scheduler(const Preferences& preferences_, Room max_rooms_, Timeslice max_timeslice_,bool option_check_forbidden_,int option_order_courses_):
        option_check_forbidden(option_check_forbidden_),
        option_order_courses(option_order_courses_),
        preferences(preferences_),
        max_rooms(max_rooms_),
        max_timeslice(max_timeslice_)
    {
        for(const auto& s : preferences)
            for(const auto& c : s.second)
            {
                auto& forbid=all_courses[c];
                {
                    for(const auto& c1 : s.second)
                        if (c!=c1)
                            forbid.insert(c1);
                }
            }
        for(const auto& c: all_courses)
        {
            ordered_courses.push_back(c.first);
        }
        if (option_order_courses)
        {
            // stable sort prevent unexpected changes to test result
            stable_sort(ordered_courses.begin(),ordered_courses.end(),[this](Course a,Course b)
            {
                return (all_courses[a].size()>all_courses[b].size()) == (option_order_courses==1);
            });
#ifndef NDEBUG
            for(const auto course: ordered_courses)
            {
                assert(ordered_courses.begin()!=ordered_courses.end() /*empty vector implies no loop*/);
                if (option_order_courses==1)
                    assert(all_courses[*ordered_courses.begin()].size()>=all_courses[course].size()/* biggest first*/);
                else
                    assert(all_courses[*ordered_courses.begin()].size()<=all_courses[course].size()/* smallest first*/);
            }
#endif // NDEBUG
        }
    }
    enum Pref_Matching
    {
        matched,
        incomplete,
        missed
    };
    Pref_Matching pref_is_ok(const set<Course> &prefs) const
    {
        set<Timeslice> occup;
        int nb_not_seen=0;
        for(const auto c: prefs)
        {
            auto found=location.find(c);
            if (found != location.end())
            {
                const auto& timeslice=found->second.second;
                if (!occup.insert(timeslice).second) // this student is already busy at this time
                    return missed;
                continue;
            }
            nb_not_seen++;
        }
        if (nb_not_seen>0)
            return incomplete;
        return matched;

    }
    Pref_Matching student_is_ok(Student s) const
    {
        return pref_is_ok(preferences.at(s)); // throw if unknown s
    }
    Pref_Matching all_is_ok() const
    {
        Pref_Matching cur=matched;
        for(const auto& s : preferences)
            switch(pref_is_ok(s.second))
            {
            case missed:
                return missed;
            case incomplete:
                cur=incomplete;
                break;
            case matched:
                break;
            }
        return cur;
    }
    void check_coherency()
    {
        for(const auto s:schedule)
        {
            const auto found=location.find(s.second);
            assert(found != location.end());
            assert(found->second == s.first);
        }
        for(const auto s:location)
        {
            const auto found=schedule.find(s.second);
            assert(found != schedule.end());
            assert(found->second == s.first);
        }
    }
    void add_course(Course c, Reservation r)
    {
        //check_coherency();
        schedule[r]=c;
        location[c]=r;

        //check_coherency();
    }
    void remove_course(Course c, Reservation r)
    {
        //check_coherency();
        schedule.erase(r);
        location.erase(c);
        //check_coherency();
    }
    bool overflow(Reservation r) const
    {
        return (r.first >= max_rooms) || (r.second >= max_timeslice) ;
    }
    Reservation next(Reservation r) const
    {
        r.second++;
        if (overflow(r))
        {
            r.second=0;
            r.first++;
        }
        return r;
    }
    bool try_here(Reservation r)
    {
        switch(all_is_ok())
        {
        case matched:
            // cout << *this << endl;
            // return false; // explore other solutions
            return true;
        case missed:
            return false;

        case incomplete:
            break;
        }

        if (overflow(r))
            return false;


        for(const auto& c: ordered_courses)
        {
            if (location.find(c)!=location.end()) // a set of residual courses will be better
                continue;
            if (option_check_forbidden)
            {
                const auto &forbid=all_courses[c];
                if ([&]()
            {
                for(int room=0 ; room < max_rooms; room++)
                    {
                        auto found = schedule.find(make_pair(room,r.second));
                        if (found!=schedule.end())
                        {
                            Course same_time=found->second;
                            if (forbid.find(same_time)!=forbid.end())
                                return true;
                        }
                    }
                    return false;
                }())
                continue;
            }
            add_course(c,r);
            if (try_here(next(r)))
                return true;
            remove_course(c,r);
        }

        return false;
    }

    bool buid_schedule()
    {
        bool succeed = try_here(Reservation{});
        POSTCONDITION(!succeed  || (all_is_ok()==matched));
        return succeed;
    }


    bool option_check_forbidden=true;
    int option_order_courses=1;

    Schedule schedule= {};
    map<Course,Reservation> location= {};
    const Preferences preferences;
    const Room max_rooms;
    const Timeslice max_timeslice;
    map<Course,set<Course>> all_courses= {};
    vector<Course> ordered_courses= {};

    friend ostream& operator<<(ostream& os, const Scheduler &s);


};

ostream& operator<<(ostream& os, const Scheduler &s)
{

    cout << "student preferences:" << endl;

    for(const auto& p : s.preferences)
    {
        cout << "student #" << p.first << "=> courses: ";
        for(const auto& c : p.second)
        {
            cout << c << " ";
        }
        cout << endl;
    }
    cout << s.max_rooms << " rooms, " << s.max_timeslice << " timeslices" << endl;
    if (s.all_is_ok()==Scheduler::matched)
    {
        cout << "schedule found:" << endl;
        for(int r=0 ; r < s.max_rooms; r++)
        {

            os << "room #" << r << " ";
            for(int t=0 ; t <  s.max_timeslice ; t++ )
            {
                auto found=s.schedule.find(make_pair(r,t));
                if (found!=s.schedule.end())
                {
                    char c=found->second;
                    os << "| " << c << " |";
                }
                else
                {
                    os << "|   |";
                }
            }
            os << endl;
        }
    }
    else
    {
        cout << "no schedule found" << endl;
    }
    return os;
}

struct Show_Duration
{
    string name;
    chrono::high_resolution_clock::time_point  before=std::chrono::high_resolution_clock::now();
    Show_Duration(const char *ch) : name(ch){}
    ~Show_Duration(){
        auto duration=chrono::duration_cast<std::chrono::microseconds>(chrono::high_resolution_clock::now()-before);
        cout << name << " duration : " << duration.count() << endl;
    }
};


void try_one(const Scheduler::Preferences & preferences_, Scheduler::Room max_rooms_, Scheduler::Timeslice max_timeslice_,int option_order_courses_)
{
    Scheduler s(preferences_, max_rooms_, max_timeslice_, true,option_order_courses_);
    {
        Show_Duration show("simple build");
        bool r=s.buid_schedule();
    }
    bool r=s.buid_schedule();
#ifndef NDEBUG
    {
        Scheduler s2(preferences_, max_rooms_, max_timeslice_, false,option_order_courses_);
        bool r2=s2.buid_schedule();
        assert(r2==r);
        if (r)
        {
            assert(s.schedule == s2.schedule);
        }
    }
#endif // NDEBUG
    cout << s;
}

void try_many(int option_order_courses_)
{
    Show_Duration show("total");
    cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    try_one({{0,{'A','B'}},{1,{'B','C'}},{2,{'C','D'}},{3,{'D','A'}}},2,2,option_order_courses_);
    cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    try_one({{0,{'A','B'}},{1,{'B','C'}},{2,{'C','D'}},{3,{'A','C'}}},2,2,option_order_courses_);
    cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;

    std::default_random_engine generator;
    generator.seed(0); // pseudo random generator with constant seed for repeatability

    for(int repeat=0; repeat<10; repeat++) // try some set of generated student preferences
    {
        Scheduler::Preferences p;
        const int nb_students=std::uniform_int_distribution<int>(5,7)(generator);
        for(int i=0; i<nb_students; i++)
        {
            const int nb_courses = std::uniform_int_distribution<int>(2,4)(generator);
            for(int j=0; j<nb_courses; j++)
            {
                Scheduler::Course c=std::uniform_int_distribution<Scheduler::Course> ('A','J')(generator);
                p[i].insert(c);
            }
        }

        for(int w=10; w>2; w--) // try same preferences with increasing time constraints
        {
            Scheduler::Preferences temp=p;
            try_one(std::move(temp),10,w,option_order_courses_);
            cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
        }

    }
}
int main()
{

    try_many(1); // decreasing order course with max constraint first
    try_many(2); // increasing order course with min constraint first
    try_many(0); // no order : alphabetical order
    return 0;
}

