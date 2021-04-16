import XCTest
@testable import LzSwift

final class LzSwiftTests: XCTestCase {
    let lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
    
    func testSimpleCompression() {
        guard let original = lorem.data(using: .utf8) else { return XCTAssert(false) }
        guard let compressed = try? original.lzipped(level: .lvl0) else { return XCTAssert(false) }
        
        XCTAssert(compressed.isLzipped)
        try? compressed.write(to: URL(fileURLWithPath: "/tmp/test.lz"))
        
        guard let uncompressed = try? compressed.lunzipped() else { return XCTAssert(false) }
        XCTAssertEqual(original, uncompressed)
    }
    
    static var allTests = [
        ("testSimpleCompression", testSimpleCompression),
    ]
}
