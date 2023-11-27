import Foundation
import lzlib

fileprivate let bufferSize = 65536

extension Lzip {
    public struct Error: Swift.Error {
        
        public enum Kind: Equatable {
            case ok
            case argument
            case memory
            case sequence
            case header
            case eof
            case data
            case library
            case unknown
        }
        
        public let kind: Kind
                
        internal init(_ code: LZ_Errno) {
            self.kind = {
                switch code {
                case LZ_ok:
                    return .ok
                case LZ_bad_argument:
                    return .data
                case LZ_mem_error:
                    return .memory
                case LZ_sequence_error:
                    return .sequence
                case LZ_header_error:
                    return .header
                case LZ_unexpected_eof:
                    return .eof
                case LZ_data_error:
                    return .data
                case LZ_library_error:
                    return .library
                default:
                    return .unknown
                }
            }()
        }
        
        public var localizedDescription: String {
            return "\(kind)"
        }
    }
}
