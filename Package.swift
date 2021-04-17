// swift-tools-version:5.2
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "LzSwift",
    products: [
        .library(name: "LzSwift", targets: ["LzSwift"]),
    ],
    targets: [
        .target(name: "lzlib"),
        .target(name: "LzSwift",dependencies: ["lzlib"]),
        .testTarget(name: "LzSwiftTests", dependencies: ["LzSwift"]),
    ],
    swiftLanguageVersions: [
        .v5,
    ]
)
