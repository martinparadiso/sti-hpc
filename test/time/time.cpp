/// @brief Basic time test
#include "clock.hpp"

#include <cassert>

int main()
{

    const auto zero = sti::datetime { 0 };
    assert(zero.epoch() == 0); // NOLINT

    const auto week_one = sti::datetime { 7, 0, 0, 0 };
    assert(week_one.epoch() == 604800); // NOLINT

    const auto one_week = sti::timedelta { 7, 0, 0, 0 };
    assert(one_week.length() == 604800); // NOLINT

    const auto two_weeks = week_one + one_week;
    assert(two_weeks.epoch() == 2 * 604800); // NOLINT

    const auto three_days = sti::timedelta { 259200 };
    assert(three_days.length() == 259200); // NOLINT
    const auto three_days_human = three_days.human();
    assert(three_days_human.days == 3); // NOLINT
    assert(three_days_human.hours == 0); // NOLINT
    assert(three_days_human.minutes == 0); // NOLINT
    assert(three_days_human.seconds == 0); // NOLINT

    return 0;
}