;#ifndef _C0C_LOG_
;#define _C0C_LOG_

MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               com0com=0x6:FACILITY_SERIAL_ERROR_CODE
              )


MessageId=0x0001 Facility=com0com Severity=Informational SymbolicName=COM0COM_LOG
Language=English
%1: %2.
.

MessageId=0x0002 Facility=com0com Severity=Informational SymbolicName=COM0COM_LOG_DRV
Language=English
%2.
.

;#endif /* _C0C_LOG_ */

