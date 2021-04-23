import Foundation
import lzlib

fileprivate let bufferSize = 65536

extension Lzip {
    public class Decompress {
        let buffer = UnsafeMutablePointer<UInt8>.allocate(capacity: bufferSize)
        var decoder: OpaquePointer?
        
        deinit {
            buffer.deallocate()
            
            LZ_decompress_close(decoder)
            decoder = nil
        }
        
        public init() {
            decoder = LZ_decompress_open()
        }
                
        public func decompress(input: Data) throws -> Data {
            return try input.withUnsafeBytes { (inBuffer: UnsafePointer<UInt8>) -> Data in
                
                var outBuffer = Pointer<UInt8>(count: bufferSize)
                var outBufferIdx = 0
                var outBufferCapacity = bufferSize
                
                let inBufferSize = input.count
                var inOffset = 0
                
                while inOffset < inBufferSize {
                    let inMaxSize = min(inBufferSize - inOffset, Int(LZ_decompress_write_size(decoder)))
                    if inMaxSize > 0 {
                        let wr = LZ_decompress_write(decoder, inBuffer + inOffset, Int32(inMaxSize))
                        if wr < 0 {
                            LZ_decompress_close(decoder)
                            decoder = nil
                            throw Lzip.Error(LZ_decompress_errno(decoder))
                        }
                        inOffset += Int(wr)
                    }
                    
                    while true {
                        
                        if outBufferIdx + bufferSize > outBufferCapacity {
                            outBufferCapacity *= 2
                            outBuffer.realloc(count: outBufferCapacity)
                        }
                        
                        let rd = LZ_decompress_read(decoder, outBuffer.baseAddress! + outBufferIdx, Int32(bufferSize))
                        if rd < 0 {
                            LZ_decompress_close(decoder)
                            decoder = nil
                            throw Lzip.Error(LZ_decompress_errno(decoder))
                        }
                        if rd <= 0 {
                            break
                        }
                        outBufferIdx += Int(rd)
                    }
                }
                                
                return outBuffer.release(count: outBufferIdx)
            }
        }
        
        private func decompressRead(output: inout Data) throws {
            while true {
                let rd = LZ_decompress_read(decoder, buffer, Int32(bufferSize))
                if rd < 0 {
                    LZ_decompress_close(decoder)
                    decoder = nil
                    throw Lzip.Error(LZ_decompress_errno(decoder))
                }
                if rd == 0 {
                    break
                }
                output.append(buffer, count: Int(rd))
            }
        }
        
        public func finish(output: inout Data) {
            LZ_decompress_finish(decoder)
            
            try? decompressRead(output: &output)
            
            LZ_decompress_close(decoder)
            decoder = nil
        }
    }
}
