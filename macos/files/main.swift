//
//  grass-launcher
//
//  Created by Nicklas Larsson on 2024-02-21.
//  (c) 2024-2025 Nicklas Larsson and the GRASS Development Team
//  SPDX-License-Identifier: GPL-2.0-or-later


import Foundation

func mainExecutableParentDir() -> URL {
    var buf = [CChar](repeating: 0, count: Int(MAXPATHLEN))
    var bufSize = UInt32(buf.count)
    let success = _NSGetExecutablePath(&buf, &bufSize) >= 0
    if !success {
        buf = [CChar](repeating: 0, count: Int(bufSize))
        let success2 = _NSGetExecutablePath(&buf, &bufSize) >= 0
        guard success2 else { fatalError() }
    }
    return URL(fileURLWithFileSystemRepresentation: buf, isDirectory: false,
               relativeTo: nil).deletingLastPathComponent()
}

var script_path = mainExecutableParentDir()
script_path.appendPathComponent("grass.scpt")

let task = Process()
task.launchPath = "/usr/bin/osascript"
task.arguments = [script_path.path]
try task.run()
