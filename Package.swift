// swift-tools-version:5.2
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "LzSwift",
    products: [
        .library(name: "LzSwift", targets: ["LzSwift"]),
    ],
    targets: [
        .target(name: "LzSwift",
                dependencies: ["lzlib"],
                linkerSettings: [
                    .unsafeFlags(["-L/usr/local/lib/"])
                ]
        ),
        .systemLibrary(
            name: "lzlib",
            path: "Sources/lzlib",
            pkgConfig: "lzlib",
            providers: [
                .brew(["lzlib"]),
                .apt(["lzlib-dev"]),
            ]
        ),
        
        .testTarget(name: "LzSwiftTests", dependencies: ["LzSwift"]),
    ],
    swiftLanguageVersions: [
        .v5,
    ]
)
