# IPC Protobuf Schema

Generate C++ sources:

```
protoc -I . --cpp_out=gen ipc_envelope.proto
```

Add the generated `gen` directory to your build include paths when enabling
`IPC_ENABLE_PROTOBUF`.
