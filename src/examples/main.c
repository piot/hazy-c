/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <hazy/hazy.h>
#include <clog/clog.h>
#include <clog/console.h>
#include <time.h>

clog_config g_clog;

int main(int argc, char *argv[])
{
    g_clog.log = clog_console;

    srand(time(NULL));

    Hazy hazy;

    Clog log;
    log.config = &g_clog.log;
    log.constantPrefix = "example";

    HazyConfig config = hazyConfigRecommended();

    hazyInit(&hazy, 1024, config, log);
    hazyWrite(&hazy, (const uint8_t*) "Hello, world!", 14);

    CLOG_INFO("done!")

    return 0;
}
