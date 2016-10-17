/*copyright (c) 2015 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __SECURITY_H__
#define __SECURITY_H__
 
#include <inttypes.h>
 
#define MBED_DOMAIN "c1893878-b3e3-4f4e-8a8c-a3b78c56a661"
#define MBED_ENDPOINT_NAME "523d7571-0506-4809-a5dc-9de857ba2022"
 
const uint8_t SERVER_CERT[] = "-----BEGIN CERTIFICATE-----\r\n"
"MIIBmDCCAT6gAwIBAgIEVUCA0jAKBggqhkjOPQQDAjBLMQswCQYDVQQGEwJGSTEN\r\n"
"MAsGA1UEBwwET3VsdTEMMAoGA1UECgwDQVJNMQwwCgYDVQQLDANJb1QxETAPBgNV\r\n"
"BAMMCEFSTSBtYmVkMB4XDTE1MDQyOTA2NTc0OFoXDTE4MDQyOTA2NTc0OFowSzEL\r\n"
"MAkGA1UEBhMCRkkxDTALBgNVBAcMBE91bHUxDDAKBgNVBAoMA0FSTTEMMAoGA1UE\r\n"
"CwwDSW9UMREwDwYDVQQDDAhBUk0gbWJlZDBZMBMGByqGSM49AgEGCCqGSM49AwEH\r\n"
"A0IABLuAyLSk0mA3awgFR5mw2RHth47tRUO44q/RdzFZnLsAsd18Esxd5LCpcT9w\r\n"
"0tvNfBv4xJxGw0wcYrPDDb8/rjujEDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0E\r\n"
"AwIDSAAwRQIhAPAonEAkwixlJiyYRQQWpXtkMZax+VlEiS201BG0PpAzAiBh2RsD\r\n"
"NxLKWwf4O7D6JasGBYf9+ZLwl0iaRjTjytO+Kw==\r\n"
"-----END CERTIFICATE-----\r\n";
 
const uint8_t CERT[] = "-----BEGIN CERTIFICATE-----\r\n"
"MIIBzzCCAXOgAwIBAgIEJCbQETAMBggqhkjOPQQDAgUAMDkxCzAJBgNVBAYTAkZ\r\n"
"JMQwwCgYDVQQKDANBUk0xHDAaBgNVBAMME21iZWQtY29ubmVjdG9yLTIwMTYwHh\r\n"
"cNMTYwNzI4MDgzNzM5WhcNMTYxMjMxMDYwMDAwWjCBoTFSMFAGA1UEAxNJYzE4O\r\n"
"TM4NzgtYjNlMy00ZjRlLThhOGMtYTNiNzhjNTZhNjYxLzUyM2Q3NTcxLTA1MDYt\r\n"
"NDgwOS1hNWRjLTlkZTg1N2JhMjAyMjEMMAoGA1UECxMDQVJNMRIwEAYDVQQKEwl\r\n"
"tYmVkIHVzZXIxDTALBgNVBAcTBE91bHUxDTALBgNVBAgTBE91bHUxCzAJBgNVBA\r\n"
"YTAkZJMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEjp/a6303UG1ZKgLO6A/IL\r\n"
"KfE0rovkEBLo5HwJIChC9DxQEwV1g2J3Rsq9zH9tV0BlQ7KUQWKwNAqPWX8R9Hz\r\n"
"hzAMBggqhkjOPQQDAgUAA0gAMEUCIAI5/j1MO5AuJaLS6hCzCovXSEwE/P+VyQw\r\n"
"QDux/u1phAiEA5kWauB4wX7hcHnTM6VLwqX4NG8o0BDL2eA2IoKLXsZw=\r\n"
"-----END CERTIFICATE-----\r\n";
 
const uint8_t KEY[] = "-----BEGIN PRIVATE KEY-----\r\n"
"MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgJv7rms8Kj2BW7HRv\r\n"
"8tkQiYI8Wi1LCdTRZEEFEbP5axehRANCAASOn9rrfTdQbVkqAs7oD8gsp8TSui+Q\r\n"
"QEujkfAkgKEL0PFATBXWDYndGyr3Mf21XQGVDspRBYrA0Co9ZfxH0fOH\r\n"
"-----END PRIVATE KEY-----\r\n";
 
#endif //__SECURITY_H__
