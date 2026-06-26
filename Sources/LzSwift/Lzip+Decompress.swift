import Foundation
import lzlib

fileprivate let bufferChunkSize = 1048576

extension Lzip {
    public class Decompress {
        let buffer = UnsafeMutablePointer<UInt8>.allocate(capacity: bufferChunkSize)
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
            return try input.withUnsafeBytes { unsafeRawBufferPointer in
                let unsafeBufferPointer = unsafeRawBufferPointer.bindMemory(to: UInt8.self)
                guard let inBuffer = unsafeBufferPointer.baseAddress else { return Data() }

                var outBufferCapacity = input.count * 100
                var outBuffer = Pointer<UInt8>(count: outBufferCapacity)
                var outBufferIdx = 0
                
                defer { outBuffer.dealloc() }
                
                let inBufferSize = input.count
                var inOffset = 0
                
                while inOffset < inBufferSize {
                    let inMaxSize = min(inBufferSize - inOffset, Int(LZ_decompress_write_size(decoder)))
                    if inMaxSize > 0 {
                        let wr = LZ_decompress_write(decoder, inBuffer + inOffset, Int32(inMaxSize))
                        if wr < 0 {
                            let err = LZ_decompress_errno(decoder)
                            LZ_decompress_close(decoder)
                            decoder = nil
                            throw Lzip.Error(err)
                        }
                        inOffset += Int(wr)
                    }
                    
                    while true {
                        
                        if outBufferIdx + bufferChunkSize > outBufferCapacity {
                            // outBufferCapacity = outBufferIdx + bufferChunkSize + 32
                            outBufferCapacity = (outBufferIdx + bufferChunkSize) * 2
                            outBuffer.realloc(count: outBufferCapacity)
                            // print("realloc: \(outBufferCapacity)")
                        }
                        
                        let rd = LZ_decompress_read(decoder, outBuffer.baseAddress! + outBufferIdx, Int32(bufferChunkSize))
                        if rd < 0 {
                            let err = LZ_decompress_errno(decoder)
                            LZ_decompress_close(decoder)
                            decoder = nil
                            throw Lzip.Error(err)
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
                let rd = LZ_decompress_read(decoder, buffer, Int32(bufferChunkSize))
                if rd < 0 {
                    let err = LZ_decompress_errno(decoder)
                    LZ_decompress_close(decoder)
                    decoder = nil
                    throw Lzip.Error(err)
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
