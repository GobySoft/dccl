// Copyright 2014-2015 Toby Schneider (https://launchpad.net/~tes)
//                     Stephanie Petillo (https://launchpad.net/~spetillo)
//                     GobySoft, LLC
//
// This file is part of the Dynamic Compact Control Language Applications
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.

#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/compiler/code_generator.h>
#include <iostream>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include "option_extensions.pb.h"
#include "gen_units_class_plugin.h"

std::set<std::string> systems_to_include_;
std::string filename_h_;


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
    void generate_message(const google::protobuf::Descriptor* desc, google::protobuf::compiler::GeneratorContext* generator_context) const;
    void generate_field(const google::protobuf::FieldDescriptor* field, google::protobuf::io::Printer* printer) const;
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
        filename_h_ = filename.substr(0, filename.find(".proto")) + ".pb.h";
//        std::string filename_cc = filename.substr(0, filename.find(".proto")) + ".pb.cc";
        
        for(int message_i = 0, message_n = file->message_type_count(); message_i < message_n; ++message_i)
        {
            generate_message(file->message_type(message_i), generator_context);
        }
        
        boost::shared_ptr<google::protobuf::io::ZeroCopyOutputStream> include_output(
            generator_context->OpenForInsert(filename_h_, "includes"));
        google::protobuf::io::Printer include_printer(include_output.get(), '$');
        std::stringstream includes_ss;

        for(std::set<std::string>::const_iterator it = systems_to_include_.begin(), end = systems_to_include_.end(); it != end; ++it)
        {
            include_units_headers(*it, includes_ss);
        }
        include_printer.Print(includes_ss.str().c_str());
        
        return true;
    }
    catch (std::exception&e)
    {
        *error = e.what();
        return false;
    }
}


void DCCLGenerator::generate_message(const google::protobuf::Descriptor* desc, google::protobuf::compiler::GeneratorContext* generator_context) const
{
    boost::shared_ptr<google::protobuf::io::ZeroCopyOutputStream> output(
        generator_context->OpenForInsert(filename_h_, "class_scope:" + desc->full_name()));
    google::protobuf::io::Printer printer(output.get(), '$');
    
    for(int field_i = 0, field_n = desc->field_count(); field_i < field_n; ++field_i)
    {
        generate_field(desc->field(field_i), &printer);
    }
}

void DCCLGenerator::generate_field(const google::protobuf::FieldDescriptor* field, google::protobuf::io::Printer* printer) const
{
    const dccl::DCCLFieldOptions& dccl_options = field->options().GetExtension(dccl::field);

    if(!dccl_options.has_units()) return;
    
    if(dccl_options.units().has_base_dimensions() && dccl_options.units().has_derived_dimensions())
    {
        throw(std::runtime_error("May define either (dccl.field).units.base_dimensions or (dccl.field).units.derived_dimensions, but not both"));
    }
    else if(dccl_options.units().has_base_dimensions())
    {

        std::stringstream new_methods;
                
        std::vector<double> powers;
        std::vector<std::string> unused;
        std::vector<std::string> dimensions;
        if(client::parse_base_dimensions(dccl_options.units().base_dimensions().begin(),
                                         dccl_options.units().base_dimensions().end(),
                                         powers, unused, dimensions))
        {
            handle_base_dims(dimensions, powers, field->name(), new_methods);

            bool is_integer = check_field_type(field);                    
            construct_field_class_plugin(is_integer,
                                         field->name(),
                                         dccl_options.units().system(),
                                         new_methods);
            printer->Print(new_methods.str().c_str());
            systems_to_include_.insert(dccl_options.units().system());
        }
        else
        {
            throw(std::runtime_error(std::string("Failed to parse base_dimensions string: \"" + dccl_options.units().base_dimensions() + "\"")));
        }
    }
    else if(dccl_options.units().has_derived_dimensions())   
    {
        std::stringstream new_methods;
                
        std::vector<std::string> operators;
        std::vector<std::string> dimensions;
        if(client::parse_derived_dimensions(dccl_options.units().derived_dimensions().begin(),
                                            dccl_options.units().derived_dimensions().end(),
                                            operators, dimensions))
        {
            handle_derived_dims(dimensions, operators, field->name(), new_methods);
                        
            bool is_integer = check_field_type(field);
            construct_field_class_plugin(is_integer,
                                         field->name(),
                                         dccl_options.units().system(),
                                         new_methods);
            printer->Print(new_methods.str().c_str());
            systems_to_include_.insert(dccl_options.units().system());
        }
        else
        {
            throw(std::runtime_error(std::string("Failed to parse base_dimensions string: \"" + dccl_options.units().base_dimensions() + "\"")));
        }
    }
}


int main(int argc, char* argv[])
{
    DCCLGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
