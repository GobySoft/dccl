// Copyright 2014-2016 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     Stephanie Petillo (http://gobysoft.org/index.wt/people/stephanie)
//                     GobySoft, LLC
//                     Community contributors (see AUTHORS file)
//
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/compiler/code_generator.h>
#include <iostream>
#include <sstream>
#include <set>
#include <boost/shared_ptr.hpp>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include "option_extensions.pb.h"
#include "gen_units_class_plugin.h"

std::set<std::string> systems_to_include_;
std::set<std::string> base_units_to_include_;
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
    void generate_message(const google::protobuf::Descriptor* desc,
                          google::protobuf::compiler::GeneratorContext* generator_context,
                          boost::shared_ptr<std::string> message_unit_system = boost::shared_ptr<std::string>()) const;
    void generate_field(const google::protobuf::FieldDescriptor* field,
                        google::protobuf::io::Printer* printer,
                        boost::shared_ptr<std::string> message_unit_system) const;
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
            try
            {
                generate_message(file->message_type(message_i), generator_context);
            }
            catch(std::exception& e)
            {
                throw(std::runtime_error(std::string("Failed to generate DCCL code: \n") + e.what()));
            }
        }
        
        boost::shared_ptr<google::protobuf::io::ZeroCopyOutputStream> include_output(
            generator_context->OpenForInsert(filename_h_, "includes"));
        google::protobuf::io::Printer include_printer(include_output.get(), '$');
        std::stringstream includes_ss;

        for(std::set<std::string>::const_iterator it = systems_to_include_.begin(), end = systems_to_include_.end(); it != end; ++it)
        {
            include_units_headers(*it, includes_ss);
        }
        for(std::set<std::string>::const_iterator it = base_units_to_include_.begin(), end = base_units_to_include_.end(); it != end; ++it)
        {
            include_base_unit_headers(*it, includes_ss);
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


void DCCLGenerator::generate_message(const google::protobuf::Descriptor* desc, google::protobuf::compiler::GeneratorContext* generator_context, boost::shared_ptr<std::string> message_unit_system) const
{
    try
    {
        boost::shared_ptr<google::protobuf::io::ZeroCopyOutputStream> output(
            generator_context->OpenForInsert(filename_h_, "class_scope:" + desc->full_name()));
        google::protobuf::io::Printer printer(output.get(), '$');

        if(desc->options().HasExtension(dccl::msg))
        {
            std::stringstream id_enum;
            id_enum << "enum DCCLParameters { DCCL_ID = " << desc->options().GetExtension(dccl::msg).id() << ", " <<
                " DCCL_MAX_BYTES = " << desc->options().GetExtension(dccl::msg).max_bytes() << " };\n";
            printer.Print(id_enum.str().c_str());


            // set message level unit system - used if fields do not specify
            const dccl::DCCLMessageOptions& dccl_msg_options = desc->options().GetExtension(dccl::msg);
            if(dccl_msg_options.has_unit_system())
            {
                message_unit_system.reset(new std::string(dccl_msg_options.unit_system()));
                systems_to_include_.insert(dccl_msg_options.unit_system());
            }
        }    
    
        for(int field_i = 0, field_n = desc->field_count(); field_i < field_n; ++field_i)
        {
            generate_field(desc->field(field_i), &printer, message_unit_system);
        }

        for(int nested_type_i = 0, nested_type_n = desc->nested_type_count(); nested_type_i < nested_type_n; ++nested_type_i)
            generate_message(desc->nested_type(nested_type_i), generator_context, message_unit_system);
    }
    catch(std::exception& e)
    {
        throw(std::runtime_error(std::string("Message: \n" + desc->full_name() + "\n" + e.what())));
    }
}

void DCCLGenerator::generate_field(const google::protobuf::FieldDescriptor* field, google::protobuf::io::Printer* printer, boost::shared_ptr<std::string> message_unit_system) const
{
    try
    {
        const dccl::DCCLFieldOptions& dccl_field_options = field->options().GetExtension(dccl::field);

        if(!dccl_field_options.has_units()) {return;}
   
        if((dccl_field_options.units().has_base_dimensions() && dccl_field_options.units().has_derived_dimensions())||
           (dccl_field_options.units().has_base_dimensions() && dccl_field_options.units().has_unit()) ||
           (dccl_field_options.units().has_unit() && dccl_field_options.units().has_derived_dimensions()))
        {
            throw(std::runtime_error("May define either (dccl.field).units.base_dimensions or (dccl.field).units.derived_dimensions or (dccl.field).units.unit, but not more than one."));
        }
        else if(dccl_field_options.units().has_unit())
        {
            std::stringstream new_methods;

            construct_units_typedef_from_base_unit(field->name(), dccl_field_options.units().unit(), dccl_field_options.units().relative_temperature(), new_methods);
            construct_field_class_plugin(field->name(),
                                         new_methods, 
                                         dccl::units::get_field_type_name(field->cpp_type()),
                                         field->is_repeated());
            printer->Print(new_methods.str().c_str());
            base_units_to_include_.insert(dccl_field_options.units().unit());
        }
        else if(dccl_field_options.units().has_base_dimensions())
        {

            std::stringstream new_methods;
                
            std::vector<double> powers;
            std::vector<std::string> short_dimensions;
            std::vector<std::string> dimensions;
            if(dccl::units::parse_base_dimensions(dccl_field_options.units().base_dimensions().begin(),
                                                  dccl_field_options.units().base_dimensions().end(),
                                                  powers, short_dimensions, dimensions))
            {
                if(!dccl_field_options.units().has_system() && !message_unit_system)
                    throw(std::runtime_error(std::string("Field must have 'system' defined or message must have 'unit_system' defined when using 'base_dimensions'.")));
              
                // default to system set in the field, otherwise use the system set at the message level
                const std::string& unit_system = (!dccl_field_options.units().has_system() && message_unit_system) ? *message_unit_system : dccl_field_options.units().system();
              
                construct_base_dims_typedef(dimensions, powers, field->name(), unit_system, dccl_field_options.units().relative_temperature(), new_methods);

                bool is_integer = check_field_type(field);                    
                construct_field_class_plugin(field->name(),
                                             new_methods, 
                                             dccl::units::get_field_type_name(field->cpp_type()),
                                             field->is_repeated());
                printer->Print(new_methods.str().c_str());
                systems_to_include_.insert(unit_system);
            }
            else
            {
                throw(std::runtime_error(std::string("Failed to parse base_dimensions string: \"" + dccl_field_options.units().base_dimensions() + "\"")));
            }
        }
        else if(dccl_field_options.units().has_derived_dimensions())   
        {
            std::stringstream new_methods;
                
            std::vector<std::string> operators;
            std::vector<std::string> dimensions;
            if(dccl::units::parse_derived_dimensions(dccl_field_options.units().derived_dimensions().begin(),
                                                     dccl_field_options.units().derived_dimensions().end(),
                                                     operators, dimensions))
            {
                if(!dccl_field_options.units().has_system() && !message_unit_system)
                    throw(std::runtime_error(std::string("Field must have 'system' defined or message must have 'unit_system' defined when using 'derived_dimensions'.")));
                const std::string& unit_system = (!dccl_field_options.units().has_system() && message_unit_system) ? *message_unit_system : dccl_field_options.units().system();
  
                construct_derived_dims_typedef(dimensions, operators, field->name(), unit_system, dccl_field_options.units().relative_temperature(), new_methods);
                        
                bool is_integer = check_field_type(field);
                construct_field_class_plugin(field->name(),
                                             new_methods, 
                                             dccl::units::get_field_type_name(field->cpp_type()),
                                             field->is_repeated());
                printer->Print(new_methods.str().c_str());
                systems_to_include_.insert(unit_system);
            }
            else
            {
                throw(std::runtime_error(std::string("Failed to parse derived_dimensions string: \"" + dccl_field_options.units().derived_dimensions() + "\"")));
            }
        }
        
    }
    catch(std::exception& e)
    {
        throw(std::runtime_error(std::string("Field: \n" + field->DebugString() + "\n" + e.what())));
    }
}


int main(int argc, char* argv[])
{
    DCCLGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
