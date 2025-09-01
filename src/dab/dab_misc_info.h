#pragma once
#include <stdint.h>
#include <map>

// NOTE: This extra information that is given that we dont' really have a need for

struct DAB_CIF_Counter {
    uint8_t upper_count = 0; // Goes up to 20
    uint8_t lower_count = 0; // Goes up to 250
    uint16_t GetTotalCount() const {
        return 
            static_cast<uint16_t>(upper_count)*250u + 
            static_cast<uint16_t>(lower_count);
    }
};

struct DAB_Datetime {
    int day = 0;
    int month = 0;
    int year = 0;
    uint8_t hours = 0;
    uint8_t minutes = 0;
    uint8_t seconds = 0;
    uint16_t milliseconds = 0;
};

struct DAB_Announcement {
    uint16_t asw_flags = 0;     // 16 bits
    uint8_t new_flag = 0;       // 1 bit
    uint8_t subchannel_id = 0;  // 6 bits
    bool operator==(const DAB_Announcement& other) const {
        return
            // (cluster_id == other.cluster_id) &&
            (asw_flags == other.asw_flags) &&
            (new_flag == other.new_flag) &&
            (subchannel_id == other.subchannel_id);
    }
    bool operator!=(const DAB_Announcement& other) const {
        return !(*this == other);
    }
};

struct DAB_Misc_Info {
    DAB_Datetime datetime;
    DAB_CIF_Counter cif_counter;
    uint8_t alarm_flag = 0;         // 1 bits
    uint8_t change_flags = 0;       // 2 bits
    uint8_t occurance_change = 0;   // 8 bits
    std::map<uint8_t, DAB_Announcement> announcements; // key => cluster_id
};