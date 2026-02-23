// ntp_service.h
#pragma once
#include <time.h>
#include <stdbool.h>

namespace NtpService {
    // Blocking NTP sync. Waits up to 10 s for valid time.
    // Returns true if time was successfully synchronised.
    bool sync();

    // Fill `out` with local time adjusted for the configured offset.
    // Returns false if the system clock is not yet set.
    bool getTime(struct tm& out);
}
