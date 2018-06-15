#include "dccl.h"
#include <iostream>
#include <memory>

// Advanced version of quick.cpp that loads the .proto at runtime, instead of compile time
// compile with g++ dccl-runtime.cpp -std=c++11 -lprotobuf -ldccl -o dccl-runtime && ./dccl-runtime

// sets Reflection fields based on type of value
void _set_field_value(const google::protobuf::Reflection* refl, const google::protobuf::FieldDescriptor* field_desc, std::unique_ptr<google::protobuf::Message>& msg,
                      double val)
{ refl->SetDouble(msg.get(), field_desc, val); }

void _set_field_value(const google::protobuf::Reflection* refl, const google::protobuf::FieldDescriptor* field_desc, std::unique_ptr<google::protobuf::Message>& msg,
                      bool val)
{ refl->SetBool(msg.get(), field_desc, val); }


void _set_field_value(const google::protobuf::Reflection* refl, const google::protobuf::FieldDescriptor* field_desc, std::unique_ptr<google::protobuf::Message>& msg,
                      int val)
{
    if(field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_ENUM)
    {
        auto enum_desc = field_desc->enum_type();
        auto enum_val_desc = enum_desc->FindValueByNumber(val);
        if(enum_val_desc)
            refl->SetEnum(msg.get(), field_desc, enum_val_desc);
    }
}

// Look up a field by name and set its value
template<typename FieldType>
void set_field(std::unique_ptr<google::protobuf::Message>& msg, const char* field_name, FieldType val)
{
    // use protobuf Reflection to fill in message       
    const google::protobuf::Reflection* refl = msg->GetReflection();
    const google::protobuf::Descriptor* desc = msg->GetDescriptor();
    const google::protobuf::FieldDescriptor* field_desc = desc->FindFieldByName(field_name);
    if(field_desc)
        _set_field_value(refl, field_desc, msg, val);
    else
        std::cerr << "Invalid field name: " << field_name << std::endl;
}

int main()
{
    // Allows runtime compilation of .proto files
    dccl::DynamicProtobufManager::enable_compilation();
    // uncomment for debugging
    // dccl::dlog.connect(dccl::logger::DEBUG1_PLUS, &std::cerr);

    // Add any -I for protoc
    dccl::DynamicProtobufManager::add_include_path("/home/toby/dccl/share/examples/quickstart_navreport");

    // FileDescriptor = type introspection on .proto
    const google::protobuf::FileDescriptor* nav_report_file_desc =
        dccl::DynamicProtobufManager::load_from_proto_file("navreport.proto");
    
    if(!nav_report_file_desc)
    {
        std::cerr << "Failed to read in navreport.proto" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    std::string encoded_bytes;
    dccl::Codec codec;

    // Descriptor = type introspection on the Message
    // we know there's only one Message in this .proto (index 0)
    const google::protobuf::Descriptor* nav_report_desc = nav_report_file_desc->message_type(0);
    codec.load(nav_report_desc);
    
    
    // SENDER
    {
        auto r_out = dccl::DynamicProtobufManager::new_protobuf_message<std::unique_ptr<google::protobuf::Message>>(nav_report_desc);
            
        set_field(r_out, "x", 450.0);
        set_field(r_out, "y", 550.0);
        set_field(r_out, "z", -100.0);
        const int AUV = 1;
        set_field(r_out, "veh_class", AUV);
        set_field(r_out, "battery_ok", true);                

        
        
        codec.encode(&encoded_bytes, *r_out);
    }
    // send encoded_bytes across your link

    // RECEIVER
    {
        auto r_in = codec.decode<std::unique_ptr<google::protobuf::Message>>(encoded_bytes);
        std::cout << r_in->ShortDebugString() << std::endl;
    }
}
