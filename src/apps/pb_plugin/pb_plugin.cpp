#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/compiler/code_generator.h>
#include <iostream>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include "dccl/protobuf/option_extensions.pb.h"
#include "gen_units_class_plugin.h"

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
    bool check_field_type(const google::protobuf::FieldDescriptor* field) const;

};

bool DCCLGenerator::check_field_type(const google::protobuf::FieldDescriptor* field) const
{    
    bool is_integer =
        field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_INT32 ||
        field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_INT64 ||
        field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_UINT32 ||
        field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_UINT64;

    bool is_float = 
        field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE ||
        field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_FLOAT;

    if(!is_float && !is_integer)
    {
        throw std::runtime_error("Can only use (dccl.field).base_dimensions on numeric fields");
    }
    return is_integer;
}


bool DCCLGenerator::Generate(const google::protobuf::FileDescriptor* file,
                            const std::string& parameter,
                             google::protobuf::compiler::GeneratorContext* generator_context,
                             std::string* error) const
{
    try
    {
        const std::string& filename = file->name();
        std::string filename_h = filename.substr(0, filename.find(".proto")) + ".pb.h";
        std::string filename_cc = filename.substr(0, filename.find(".proto")) + ".pb.cc";
    
        for(int message_i = 0, message_n = file->message_type_count(); message_i < message_n; ++message_i)
        {
            const google::protobuf::Descriptor * desc = file->message_type(message_i);
            boost::shared_ptr<google::protobuf::io::ZeroCopyOutputStream> output(
                generator_context->OpenForInsert(filename_h, "class_scope:" + desc->full_name()));
            google::protobuf::io::Printer printer(output.get(), '$');
        
            for(int field_i = 0, field_n = desc->field_count(); field_i < field_n; ++field_i)
            {
                const google::protobuf::FieldDescriptor * field = desc->field(field_i);

                const dccl::DCCLFieldOptions& dccl_options = field->options().GetExtension(dccl::field);

                if(dccl_options.has_base_dimensions() && dccl_options.has_derived_dimensions())
                {
                    throw(std::runtime_error("May define either (dccl.field).base_dimensions or (dccl.field).derived_dimensions, but not both"));
                }
                else if(dccl_options.has_base_dimensions())
                {
                    std::stringstream new_methods;
                
                    std::vector<double> powers;
                    std::vector<std::string> unused;
                    std::vector<std::string> dimensions;
                    if(client::parse_base_dimensions(dccl_options.base_dimensions().begin(),
                                                     dccl_options.base_dimensions().end(),
                                                     powers, unused, dimensions))
                    {
                        handle_base_dims(dimensions, powers, field->name(), new_methods);

                        bool is_integer = check_field_type(field);
                    
                        construct_field_class_plugin(is_integer,
                                                     field->name(),
                                                     dccl_options.units_system(),
                                                     new_methods);
                    
                        printer.Print(new_methods.str().c_str());
                    }
                    else
                    {
                        throw(std::runtime_error(std::string("Failed to parse base_dimensions string: \"" + dccl_options.base_dimensions() + "\"")));
                    }
                }
                else if(dccl_options.has_derived_dimensions())   
                {
                
                }
            }
        }
        return true;
    }
    catch (std::exception&e)
    {
        *error = e.what();
        return false;
    }
}


int main(int argc, char* argv[])
{
    DCCLGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
