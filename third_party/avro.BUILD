licenses(["notice"])

# copy headers to directory avro so that they will be referenced as avro/x.hh, which makes cpplint happy
genrule(
    name = "api_hdrs",
    srcs = glob(
        ["lang/c++/api/**/*.hh"],
    ),
    outs = [
        "avro/AvroTraits.hh",
        "avro/Boost.hh",
        "avro/Compiler.hh",
        "avro/Config.hh",
        "avro/DataFile.hh",
        "avro/Decoder.hh",
        "avro/Encoder.hh",
        "avro/Exception.hh",
        "avro/Generic.hh",
        "avro/GenericDatum.hh",
        "avro/Layout.hh",
        "avro/Node.hh",
        "avro/NodeConcepts.hh",
        "avro/NodeImpl.hh",
        "avro/Reader.hh",
        "avro/Resolver.hh",
        "avro/ResolverSchema.hh",
        "avro/Schema.hh",
        "avro/SchemaResolution.hh",
        "avro/Specific.hh",
        "avro/Stream.hh",
        "avro/Types.hh",
        "avro/Validator.hh",
        "avro/ValidSchema.hh",
        "avro/Zigzag.hh",
        "avro/buffer/Buffer.hh",
        "avro/buffer/BufferReader.hh",
        "avro/buffer/detail/BufferDetail.hh",
        "avro/buffer/detail/BufferDetailIterator.hh",
    ],
    cmd = "cp -r external/" + REPOSITORY_NAME[1:] + "/lang/c++/api/*.hh $(@D)/avro/;" +
        "cp -r external/" + REPOSITORY_NAME[1:] + "/lang/c++/api/buffer/*.hh $(@D)/avro/buffer/;" +
        "cp -r external/" + REPOSITORY_NAME[1:] + "/lang/c++/api/buffer/detail/*.hh $(@D)/avro/buffer/detail/",
)

cc_library(
    name = "avro",
    srcs = [
        ":api_hdrs",
        "lang/c++/impl/BinaryDecoder.cc",
        "lang/c++/impl/BinaryEncoder.cc",
        "lang/c++/impl/Compiler.cc",
        "lang/c++/impl/DataFile.cc",
        "lang/c++/impl/FileStream.cc",
        "lang/c++/impl/Generic.cc",
        "lang/c++/impl/GenericDatum.cc",
        "lang/c++/impl/Node.cc",
        "lang/c++/impl/NodeImpl.cc",
        "lang/c++/impl/Resolver.cc",
        "lang/c++/impl/ResolverSchema.cc",
        "lang/c++/impl/Schema.cc",
        "lang/c++/impl/Stream.cc",
        "lang/c++/impl/Types.cc",
        "lang/c++/impl/ValidSchema.cc",
        "lang/c++/impl/Validator.cc",
        "lang/c++/impl/Zigzag.cc",
        "lang/c++/impl/json/JsonIO.hh",
        "lang/c++/impl/json/JsonIO.cc",
        "lang/c++/impl/json/JsonDom.hh",
        "lang/c++/impl/json/JsonDom.cc",
        "lang/c++/impl/parsing/JsonCodec.cc",
        "lang/c++/impl/parsing/ResolvingDecoder.cc",
        "lang/c++/impl/parsing/Symbol.hh",
        "lang/c++/impl/parsing/Symbol.cc",
        "lang/c++/impl/parsing/ValidatingCodec.hh",
        "lang/c++/impl/parsing/ValidatingCodec.cc",
    ],
    includes = [
        "avro",
        "avro/buffer",
    ],
    copts = [
        "-Wno-deprecated-declarations",
    ],
    deps = [
        "//external:zlib",
        "//external:boost",
        "//external:boost_iostreams",
    ],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "avrogen",
    srcs = [
        "lang/c++/impl/avrogencpp.cc",
    ],
    deps = [
        ":avro",
        "//external:boost",
        "//external:boost_program_options",
    ],
)
