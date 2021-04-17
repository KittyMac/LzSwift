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
        var destination = Data(capacity: self.count * 4)
        try compressor.compress(input: self,
                                output: &destination)
        compressor.finish(output: &destination)
        return destination
    }
    
    public func lunzipped() throws -> Data? {
        let decompressor = Lzip.Decompress()
        var destination = Data(capacity: self.count / 2)
        try decompressor.decompress(input: self,
                                    output: &destination)
        decompressor.finish(output: &destination)
        return destination
    }

}

