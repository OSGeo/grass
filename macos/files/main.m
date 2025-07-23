//
//  grass-launcher-objc
//
//  Created by Nicklas Larsson on 2024-02-27.
//  (c) 2024-2025 Nicklas Larsson and the GRASS Development Team
//  SPDX-License-Identifier: GPL-2.0-or-later

#import <Foundation/Foundation.h>
#import <mach-o/dyld.h>

NSURL *mainExecutableParentDir(void) {
    NSURL *url;
    char *buf = (char *)malloc(sizeof(char) * MAXPATHLEN);
    uint32_t bufSize = sizeof(buf);
    if (_NSGetExecutablePath(buf, &bufSize) < 0) {
        free(buf);
        char *buf = (char *)malloc(sizeof(char) * bufSize);
        if (_NSGetExecutablePath(buf, &bufSize) < 0) {
            free(buf);
            return nil;
        }
        url = [NSURL fileURLWithFileSystemRepresentation:buf
                                             isDirectory:NO
                                           relativeToURL:nil];
        free(buf);
    } else {
        url = [NSURL fileURLWithFileSystemRepresentation:buf
                                             isDirectory:NO
                                           relativeToURL:nil];
        free(buf);
    }
    return url.URLByDeletingLastPathComponent;
}

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        NSURL *script_path = mainExecutableParentDir();
        if (!script_path) {
            return 1;
        }
        script_path = [script_path URLByAppendingPathComponent:@"grass.scpt"];

        NSTask *task = [[NSTask alloc] init];
        task.executableURL = [NSURL fileURLWithPath:@"/usr/bin/osascript"
                                        isDirectory:NO];
        task.arguments = @[ script_path.path ];
        [task launch];
    }
    return 0;
}
