import Foundation
import lzlib

public enum Lzip { }

extension Lzip {
    public enum CompressionLevel {
        case lvl0
        case lvl1
        case lvl2
        case lvl3
        case lvl4
        case lvl5
        case lvl6
        case lvl7
        case lvl8
        case lvl9
    }
}

extension Data {
    public var isLzipped: Bool {
        return self.starts(with: [0x4c, 0x5a, 0x49, 0x50])
    }

    public func lzipped(level: Lzip.CompressionLevel) throws -> Data? {
        let compressor = Lzip.Compress(level: level)
        var destination = try compressor.compress(input: self)
        compressor.finish(output: &destination)
        return destination
    }
    
    public func lunzipped() throws -> Data? {
        let decompressor = Lzip.Decompress()
        var destination = try decompressor.decompress(input: self)
        decompressor.finish(output: &destination)
        return destination
    }
}

class Pointer<T: ExpressibleByIntegerLiteral> {
    var baseAddress: UnsafeMutablePointer<T>?
    var count: Int
    
    init (count: Int) {
        self.count = count
        self.baseAddress = malloc(count * MemoryLayout<T>.size)?.assumingMemoryBound(to: T.self)
    }
        
    func dealloc() {
        baseAddress?.deallocate()
        baseAddress = nil
    }
    
    func realloc(count: Int) {
        guard let baseAddress = baseAddress else { return }
        let newSize = count * MemoryLayout<T>.size
        self.baseAddress = Foundation.realloc(baseAddress, count * MemoryLayout<T>.size)?.assumingMemoryBound(to: T.self)
        self.count = newSize
    }
    
    func release(count: Int) -> Data {
        guard let baseAddress = baseAddress else { return Data() }
        let data = Data(bytesNoCopy: baseAddress, count: count, deallocator: .free)
        self.baseAddress = nil
        return data
    }
    
    func export(count: Int) -> Data {
        guard let baseAddress = baseAddress else { return Data() }
        return Data(bytes: baseAddress, count: count)
    }
}

