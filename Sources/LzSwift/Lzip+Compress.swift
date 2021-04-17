import Foundation
import lzlib

fileprivate let bufferSize = 16384

extension Lzip {
    public class Compress {
        let buffer = UnsafeMutablePointer<UInt8>.allocate(capacity: bufferSize)
        var encoder: OpaquePointer?
        
        deinit {
            buffer.deallocate()
            
            LZ_compress_close(encoder)
            encoder = nil
        }
        
        public init(level: CompressionLevel) {
            var dicationarySize: Int32 = 65535
            var matchLenLimit: Int32 = 16
            
            switch level {
            case .lvl0:
                dicationarySize = 65535
                matchLenLimit = 16
            case .lvl1:
                dicationarySize = 1 << 20
                matchLenLimit = 5
            case .lvl2:
                dicationarySize = 1 << 19
                matchLenLimit = 6
            case .lvl3:
                dicationarySize = 1 << 21
                matchLenLimit = 8
            case .lvl4:
                dicationarySize = 1 << 20
                matchLenLimit = 12
            case .lvl5:
                dicationarySize = 1 << 22
                matchLenLimit = 20
            case .lvl6:
                dicationarySize = 1 << 23
                matchLenLimit = 36
            case .lvl7:
                dicationarySize = 1 << 24
                matchLenLimit = 68
            case .lvl8:
                dicationarySize = 1 << 23
                matchLenLimit = 132
            case .lvl9:
                dicationarySize = 1 << 25
                matchLenLimit = 273
            }
            
            encoder = LZ_compress_open(dicationarySize, matchLenLimit, UInt64.max)
        }
        
        private func compressRead(output: inout Data) throws {
            while true {
                let rd = LZ_compress_read(encoder, buffer, Int32(bufferSize))
                if rd < 0 {
                    LZ_compress_close(encoder)
                    encoder = nil
                    throw Lzip.Error(LZ_compress_errno(encoder))
                }
                if rd == 0 {
                    break
                }
                output.append(buffer, count: Int(rd))
            }
        }
                
        public func compress(input: Data,
                             output: inout Data) throws {
            try input.withUnsafeBytes { (inBuffer: UnsafePointer<UInt8>) -> Void in
                let inBufferSize = input.count
                var inOffset = 0
                
                while inOffset < inBufferSize {
                    let inMaxSize = min(inBufferSize - inOffset, Int(LZ_compress_write_size(encoder)))
                    if inMaxSize > 0 {
                        let wr = LZ_compress_write(encoder, inBuffer + inOffset, Int32(inMaxSize))
                        if wr < 0 {
                            LZ_compress_close(encoder)
                            encoder = nil
                            throw Lzip.Error(LZ_compress_errno(encoder))
                        }
                        inOffset += Int(wr)
                    }
                    try compressRead(output: &output)
                }
            }
        }
        
        public func finish(output: inout Data) {
            LZ_compress_finish(encoder)
            
            try? compressRead(output: &output)
            
            LZ_compress_close(encoder)
            encoder = nil
        }
    }
}
