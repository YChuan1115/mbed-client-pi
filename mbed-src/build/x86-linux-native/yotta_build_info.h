#ifndef __YOTTA_BUILD_INFO_H__
#define __YOTTA_BUILD_INFO_H__
// yotta build info, #include YOTTA_BUILD_INFO_HEADER to access
#define YOTTA_BUILD_YEAR 2016 // UTC year
#define YOTTA_BUILD_MONTH 10 // UTC month 1-12
#define YOTTA_BUILD_DAY 17 // UTC day 1-31
#define YOTTA_BUILD_HOUR 3 // UTC hour 0-24
#define YOTTA_BUILD_MINUTE 52 // UTC minute 0-59
#define YOTTA_BUILD_SECOND 14 // UTC second 0-61
#define YOTTA_BUILD_UUID d1b81731-462d-4cc6-af21-ca71c502394b // unique random UUID for each build
#define YOTTA_BUILD_VCS_ID 86be4858299ed2b7298093d15695a2cb59a40919 // git or mercurial hash
#define YOTTA_BUILD_VCS_CLEAN 0 // evaluates true if the version control system was clean, otherwise false
#define YOTTA_BUILD_VCS_DESCRIPTION mbedos-16.01-release-2-g86be485 // git describe or mercurial equivalent
#endif // ndef __YOTTA_BUILD_INFO_H__
