import XCTest
@testable import LzSwift

final class LzSwiftTests: XCTestCase {
    let lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
    
    func testSimpleCompression() {
        guard let original = lorem.data(using: .utf8) else { return XCTAssert(false) }
        guard let compressed = try? original.lzipped(level: .lvl0) else { return XCTAssert(false) }
        
        XCTAssert(compressed.isLzipped)
        
        guard let uncompressed = try? compressed.lunzipped() else { return XCTAssert(false) }
        XCTAssertEqual(original, uncompressed)
    }
    
    func testStreamedCompression() {
        let originals = [
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
            " Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.",
            " Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.",
            " Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
        ]
        
        let compressor = Lzip.Compress(level: .lvl0)
        var compressed = Data()
        for original in originals {
            guard let data = original.data(using: .utf8) else { return XCTAssert(false) }
            try? compressor.compress(input: data, output: &compressed)
        }
        compressor.finish(output: &compressed)
        
        XCTAssert(compressed.isLzipped)
        
        guard let uncompressed = try? compressed.lunzipped() else { return XCTAssert(false) }
        XCTAssertEqual(lorem, String(data: uncompressed, encoding: .utf8))
    }
    
    func testStreamedDecompression() {
        guard let original = lorem.data(using: .utf8) else { return XCTAssert(false) }
        guard let compressed = try? original.lzipped(level: .lvl0) else { return XCTAssert(false) }
        
        XCTAssert(compressed.isLzipped)
        
        let decompressor = Lzip.Decompress()
        var decompressed = Data()
        
        let parts = [
            compressed.subdata(in: 0..<20),
            compressed.subdata(in: 20..<40),
            compressed.subdata(in: 40..<60),
            compressed.subdata(in: 60..<100),
            compressed.subdata(in: 100..<compressed.count)
        ]
        
        for part in parts {
            try? decompressor.decompress(input: part, output: &decompressed)
        }
        decompressor.finish(output: &decompressed)
        
        XCTAssertEqual(lorem, String(data: decompressed, encoding: .utf8))
    }
    
    static var allTests = [
        ("testSimpleCompression", testSimpleCompression),
    ]
}
