import Foundation
import lzlib

fileprivate let bufferSize = 16384

extension Lzip {
    public class Decompress {
        let buffer = UnsafeMutablePointer<UInt8>.allocate(capacity: bufferSize)
        var decoder: OpaquePointer?
        
        deinit {
            buffer.deallocate()
        }
        
        public init() {
            decoder = LZ_decompress_open()
        }
        
        private func decompressRead(output: inout Data) throws {
            while true {
                let rd = LZ_decompress_read(decoder, buffer, Int32(bufferSize))
                if rd < 0 {
                    LZ_decompress_close(decoder)
                    throw Lzip.Error(LZ_decompress_errno(decoder))
                }
                if rd == 0 {
                    break
                }
                output.append(buffer, count: Int(rd))
            }
        }
                
        public func decompress(input: Data,
                               output: inout Data) throws {
            try input.withUnsafeBytes { (inBuffer: UnsafePointer<UInt8>) -> Void in
                let inBufferSize = input.count
                var inOffset = 0
                
                while inOffset < inBufferSize {
                    let inMaxSize = min(inBufferSize - inOffset, Int(LZ_decompress_write_size(decoder)))
                    if inMaxSize > 0 {
                        let wr = LZ_decompress_write(decoder, inBuffer + inOffset, Int32(inMaxSize))
                        if wr < 0 {
                            LZ_decompress_close(decoder)
                            throw Lzip.Error(LZ_decompress_errno(decoder))
                        }
                        inOffset += Int(wr)
                    }
                    
                    try decompressRead(output: &output)
                }
            }
        }
        
        public func finish(output: inout Data) {
            LZ_decompress_finish(decoder)
            
            try? decompressRead(output: &output)
            
            LZ_decompress_close(decoder)
        }
    }
}
