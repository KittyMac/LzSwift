import Foundation
import lzlib

extension Lzip {
    public class Decompress {
        var buffer: Pointer<UInt8>
        var decoder: OpaquePointer?
        
        deinit {
            buffer.dealloc()
            LZ_decompress_close(decoder)
            decoder = nil
        }
        
        public init(bufferSize: Int = 65536) {
            buffer = Pointer<UInt8>(count: bufferSize)
            decoder = LZ_decompress_open()
        }
                
        public func decompress(input: Data) throws -> Data {
            return try input.withUnsafeBytes { unsafeRawBufferPointer in
                let unsafeBufferPointer = unsafeRawBufferPointer.bindMemory(to: UInt8.self)
                guard let inBuffer = unsafeBufferPointer.baseAddress else { return Data() }
                
                let inBufferSize = input.count
                var inOffset = 0
                
                var bufferIdx = 0
                
                while inOffset < inBufferSize {
                    let inMaxSize = min(inBufferSize - inOffset, Int(LZ_decompress_write_size(decoder)))
                    if inMaxSize > 0 {
                        let wr = LZ_decompress_write(decoder, inBuffer + inOffset, Int32(inMaxSize))
                        if wr < 0 {
                            buffer.dealloc()
                            LZ_decompress_close(decoder)
                            decoder = nil
                            throw Lzip.Error(LZ_decompress_errno(decoder))
                        }
                        inOffset += Int(wr)
                    }
                    
                    while buffer.baseAddress != nil {
                        
                        if bufferIdx + buffer.count > buffer.count {
                            buffer.realloc(count: bufferIdx + buffer.count)
                        }
                        
                        let availableSpace = buffer.count - bufferIdx
                        let rd = LZ_decompress_read(decoder, buffer.baseAddress! + bufferIdx, Int32(availableSpace))
                        if rd < 0 {
                            buffer.dealloc()
                            LZ_decompress_close(decoder)
                            decoder = nil
                            throw Lzip.Error(LZ_decompress_errno(decoder))
                        }
                        if rd < availableSpace {
                            bufferIdx += Int(rd)
                            break
                        }
                        bufferIdx += Int(rd)
                    }
                }
                
                return buffer.export(count: bufferIdx)
            }
        }
        
        private func decompressRead(output: inout Data) throws {
            while buffer.baseAddress != nil {
                let rd = LZ_decompress_read(decoder, buffer.baseAddress!, Int32(buffer.count))
                if rd < 0 {
                    buffer.dealloc()
                    LZ_decompress_close(decoder)
                    decoder = nil
                    throw Lzip.Error(LZ_decompress_errno(decoder))
                }
                if rd == 0 {
                    break
                }
                output.append(buffer.baseAddress!, count: Int(rd))
            }
        }
        
        public func finish(output: inout Data) {
            LZ_decompress_finish(decoder)
            
            try? decompressRead(output: &output)
            
            buffer.dealloc()
            LZ_decompress_close(decoder)
            decoder = nil
        }
    }
}
