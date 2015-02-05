#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/compiler/code_generator.h>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>

class DCCLGenerator : public google::protobuf::compiler::CodeGenerator {
 public:
    DCCLGenerator() { }
    ~DCCLGenerator() { }
    

  // implements CodeGenerator ----------------------------------------
    bool Generate(const google::protobuf::FileDescriptor* file,
                  const std::string& parameter,
                  google::protobuf::compiler::GeneratorContext* generator_context,
                  std::string* error) const;

 private:
};

bool DCCLGenerator::Generate(const google::protobuf::FileDescriptor* file,
                            const std::string& parameter,
                             google::protobuf::compiler::GeneratorContext* generator_context,
                             std::string* error) const {

    boost::shared_ptr<google::protobuf::io::ZeroCopyOutputStream> output(
        generator_context->OpenForInsert("test.pb.h", "class_scope:Range"));
    google::protobuf::io::Printer printer(output.get(), '$');
    printer.PrintRaw("FOOBAZ");
    
    return true;
}

int main(int argc, char* argv[])
{
    DCCLGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
