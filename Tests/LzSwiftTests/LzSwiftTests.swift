import XCTest
@testable import LzSwift

final class LzSwiftTests: XCTestCase {
    let lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
    
    func runPerformance(_ expectedSize: Int, _ file: String) {
        // All in memory compression / decompression test
        do {
            let inputURL = URL(fileURLWithPath: file)
            let data = try Data(contentsOf: inputURL)
            var output = Data()
            
            if data.isLzipped {
                let decompressor = Lzip.Decompress()
                output = try decompressor.decompress(input: data)
            } else {
                let compressor = Lzip.Compress(level: .lvl6)
                output = try compressor.compress(input: data)
                compressor.finish(output: &output)
            }
                        
            XCTAssertEqual(expectedSize, output.count)
            
        } catch {
            print("failed: \(error)")
        }
    }
    
    func testPerformanceCompression() {
        // 9.48 GB -> 197 MB
        runPerformance(196_971_818, "/Volumes/Optane/ClusterArchiver/testFile1.csv")
    }
    
    func testPerformanceDecompression() {
        // 197 MB -> 9.48 GB
        // 44.3 secs (lzip CLI 54.648 secs)
        runPerformance(9_484_439_864, "/Volumes/Optane/ClusterArchiver/testFile1.csv.lz")
    }
    
    
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
            compressed.append(try! compressor.compress(input: data))
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
            if let data = try? decompressor.decompress(input: part) {
                decompressed.append(data)
            } else {
                XCTFail()
            }
        }
        decompressor.finish(output: &decompressed)
        
        XCTAssertEqual(lorem, String(data: decompressed, encoding: .utf8))
    }
    
    static var allTests = [
        ("testSimpleCompression", testSimpleCompression),
    ]
}
