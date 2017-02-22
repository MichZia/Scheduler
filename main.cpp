

#include <map>
#include <set>
#include <iostream>
#include <cassert>

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

    Scheduler(Preferences&& preferences_, Room max_rooms_, Timeslice max_timeslice_):
        preferences(preferences_),
        max_rooms(max_rooms_),
        max_timeslice(max_timeslice_)
    {
        for(const auto& s : preferences)
            for(const auto& c : s.second)
                all_courses.insert(c);
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
    void add_course(Course c, Reservation r)
    {
        schedule[r]=c;
        location[c]=r;
    }
    void remove_course(Course c, Reservation r)
    {
        schedule.erase(r);
        location.erase(c);
    }
    Reservation next(Reservation r) const
    {
        r.second++;
        if (r.second >= max_timeslice)
        {
            r.second=0;
            r.first++;
        }
        return r;
    }
    bool last(Reservation r) const
    {
        return (r.first >= max_rooms);
    }
    bool try_here(Reservation r)
    {
        if (last(r))
            return false;
        for(const auto& c: all_courses)
        {
            if (location.find(c)!=location.end()) // a set of residual courses will be better
                continue;

            add_course(c,r);
            switch(all_is_ok())
            {
            case matched:
                return true;

            case incomplete:
                if (try_here(next(r)))
                    return true;
                break;
            case missed:
                break;
            }
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

    Schedule schedule={};
    map<Course,Reservation> location={};
    const Preferences preferences;
    const Room max_rooms;
    const Timeslice max_timeslice;
    set<Course> all_courses={};
};

ostream& operator<<(ostream& os, const Scheduler &s)
{

    cout << "preferences:" << endl;
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

void try_one(Scheduler::Preferences && preferences_, Scheduler::Room max_rooms_, Scheduler::Timeslice max_timeslice_)
{
    Scheduler s(forward<Scheduler::Preferences >(preferences_), max_rooms_, max_timeslice_);
    s.buid_schedule();
    cout << s;
}

int main()
{
    try_one({{0,{'A','B'}},{1,{'B','C'}},{2,{'C','D'}},{3,{'D','A'}}},2,2);
    try_one({{0,{'A','B'}},{1,{'B','C'}},{2,{'C','D'}},{3,{'A','C'}}},2,2);
    return 0;
}
