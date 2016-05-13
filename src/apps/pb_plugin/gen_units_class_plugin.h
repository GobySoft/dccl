// Copyright 2015-2016 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
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
#ifndef GenUnitsClassPlugin20150310H
#define GenUnitsClassPlugin20150310H

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/qi_expect.hpp>
#include <boost/lambda/lambda.hpp>

#include <boost/algorithm/string/replace.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/bimap.hpp>

///////////////////////////////////////////////////////////////////////////////////
// Parsing methods for protobuf messages' units fields:
// base_dimensions, derived_dimensions, unit_system
///////////////////////////////////////////////////////////////////////////////////

namespace dccl
{
  namespace units
  {
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;
    namespace phoenix = boost::phoenix;

    // Create a bimap of the available base dimensions
    inline boost::bimap< std::string,char > make_dim_bimap()
    {
      typedef boost::bimap< std::string, char > dim_bimap;
      typedef dim_bimap::value_type dimension;

      dim_bimap dims;
      dims.insert( dimension("length",             'L') );
      dims.insert( dimension("time",               'T') );
      dims.insert( dimension("mass",               'M') );
      dims.insert( dimension("plane_angle",        'A') );
      dims.insert( dimension("solid_angle",        'S') );
      dims.insert( dimension("current",            'I') );
      dims.insert( dimension("temperature",        'K') );
      dims.insert( dimension("amount",             'N') );
      dims.insert( dimension("luminous_intensity", 'J') );
      dims.insert( dimension("information",        'B') );
      dims.insert( dimension("dimensionless",      '-') );

      return dims;
    }

    // Create vectors of base dimension long and short strings from short (char) inputs
    inline void push_char_base(std::vector<std::string>& vc, std::vector<std::string>& vs, const char& c)
    {
      vc.push_back(std::string(1, c));

      typedef boost::bimap< std::string, char > bimap_type;
      bimap_type dim_bimap = make_dim_bimap();

      bimap_type::right_const_iterator it_right = dim_bimap.right.find(c);

      vs.push_back(it_right -> second);
    }

    // Create vectors of base dimension long and short strings from long (string) inputs
    inline void push_string_base(std::vector<std::string>& vc, std::vector<std::string>& vs, const std::string& s)
    {
      vs.push_back(s);

      typedef boost::bimap< std::string, char > bimap_type;
      bimap_type dim_bimap = make_dim_bimap();

      bimap_type::left_const_iterator it_left = dim_bimap.left.find(s);

      vc.push_back(std::string(1, it_left -> second));
    }

    // Make a vector of strings from char inputs (for creating the derived_dimensions operator vector)
    inline void push_char(std::vector<std::string>& vc, const char& c)
    {
      vc.push_back(std::string(1, c));
    }

    // Make a vector of strings from vector<char> inputs (for creating the derived_dimensions vector of strings)
    inline void push_char_vec(std::vector<std::string>& vc, const std::vector<char>& c)
    {
      vc.push_back(std::string(c.begin(), c.end()));
    }

    // Parser for base_dimensions input
    template <typename Iterator>
      bool parse_base_dimensions(Iterator first, Iterator last, std::vector<double>& base_dim_powers, std::vector<std::string>& base_dim_chars, std::vector<std::string>& base_dim_strings)
      {    

	//base_dim_chars = vc;
	//base_dim_strings = vs;
	//base_dim_powers = v;

	using qi::double_;
	using qi::char_;
	using qi::_1;
	using qi::phrase_parse;
	using ascii::space;
	using phoenix::push_back;
	using qi::eps;

	try 
	  {

	    bool r = phrase_parse(
				  first,                          /*< start iterator >*/
				  last,                           /*< end iterator >*/
				  +((char_("LTMASIKNJB-")[phoenix::bind(&push_char_base, phoenix::ref(base_dim_chars), phoenix::ref(base_dim_strings), _1)] |
				     ((ascii::string("length") | ascii::string("time") | ascii::string("mass") | ascii::string("plane_angle") | ascii::string("solid_angle") | ascii::string("current") | ascii::string("temperature") | ascii::string("amount") | ascii::string("luminous_intensity") | ascii::string("information") | ascii::string("dimensionless"))[phoenix::bind(&push_string_base, phoenix::ref(base_dim_chars), phoenix::ref(base_dim_strings), _1)])) >>
				    -(ascii::string("_base_dimension")) >>
				    (('^' > double_[push_back(phoenix::ref(base_dim_powers), _1)]) | eps[push_back(phoenix::ref(base_dim_powers), 1)])),
				  space                           /*< the skip-parser >*/
				  );

	    if (first != last) // fail if we did not get a full match
	      return false;
	    return r;
	  }
	catch(const std::runtime_error& e)
	  {
	    return false;
	  }

      }

    // Parser for derived_dimensions input
    template <typename Iterator>
      bool parse_derived_dimensions(Iterator first, Iterator last, std::vector<std::string>& derived_dim_operators, std::vector<std::string>& derived_dim_strings)
      {
	using qi::double_;
	using qi::char_;
	using qi::_1;
	using qi::phrase_parse;
	using ascii::space;
	using phoenix::push_back;
	using qi::eps;

	try 
	  {
	    std::vector<std::string> params;
	    bool r = boost::spirit::qi::parse(first, last,
					      +((+char_("a-z1_"))[phoenix::bind(&push_char_vec, boost::phoenix::ref(params), _1)] >>
						-(*char_(" ") >> (char_("*/")[phoenix::bind(&push_char, phoenix::ref(derived_dim_operators), _1)] | eps[push_back(phoenix::ref(derived_dim_operators), "*")]) >> *char_(" "))));

	    if(derived_dim_operators.size())
	      derived_dim_operators.pop_back();
    
	    for(std::vector<std::string>::iterator it = params.begin(), end = params.end(); it!=end; ++it)
	      {
		std::string::size_type dim_pos = it->find("_dimension");
		if(dim_pos != std::string::npos)
		  *it = it->substr(0, dim_pos);
		//std::cout << *it << std::endl;
		derived_dim_strings.push_back(*it);
	      }

	    if (first != last) // fail if we did not get a full match
	      return false;
	    return r;
	  }
	catch(const std::runtime_error& e)
	  {
	    return false;
	  }

      }

    // Extract field type name as string
    std::string get_field_type_name(google::protobuf::FieldDescriptor::CppType type){
      switch(type){
      case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
	return "google::protobuf::int32";
      case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
	return "google::protobuf::int64";
      case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
	return "google::protobuf::uint32";
      case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
	return "google::protobuf::uint64";
      case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
	return "double";
      case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
	return "float";
      default:
	return "double";
      }
    }

  }
}


///////////////////////////////////////////////////////////////////////////////////
// Print units class plugin pieces to some type of ostream
///////////////////////////////////////////////////////////////////////////////////

// Example:
// ClassName (Range)
// fieldname (distance)
// sysname (si)
// dimtype_dimension (length_dimension)

// Boost Units - Dimensions Reference:
// http://www.boost.org/doc/libs/1_57_0/doc/html/boost_units/Reference.html#dimensions_reference

// Generate necessary units systems headers
inline void include_units_headers(const std::string& sysname, std::ostream& os){
  os << std::endl;
  /* os <<std::endl; */
  /* os <<"//===================" <<std::endl; */
  /* os <<std::endl; */

  // pre-defined systems from boost units:
  // http://www.boost.org/doc/libs/1_54_0/boost/units/systems/

  static bool output_extra_headers = false;
  if(!output_extra_headers)
    {
      os <<"#include <boost/units/absolute.hpp>" <<std::endl;
      os <<"#include <boost/units/dimensionless_type.hpp>" <<std::endl;
      output_extra_headers = true;
    }

  if(sysname == "si" ||
     sysname == "boost::units::si" ||
     sysname == "angle::radian")
    {
      os <<"#include <boost/units/systems/si.hpp>" <<std::endl;
    }
  else if(sysname == "cgs" ||
          sysname == "boost::units::cgs")
    {
      os <<"#include <boost/units/systems/cgs.hpp>" <<std::endl;
    }
  else if(sysname == "celsius" ||
          sysname == "boost::units::celsius" ||
          sysname == "temperature::celsius")
    {
      os <<"#include <boost/units/systems/temperature/celsius.hpp>" <<std::endl;
    }
  else if(sysname == "fahrenheit" ||
          sysname == "boost::units::fahrenheit" ||
          sysname == "temperature::fahrenheit")
    {
      os <<"#include <boost/units/systems/temperature/fahrenheit.hpp>" <<std::endl;
    }
  else if(sysname == "degree" ||
          sysname == "boost::units::degree" ||
          sysname == "angle::degree")
    {
      os <<"#include <boost/units/systems/angle/degrees.hpp>" <<std::endl;
    }
  else if(sysname == "gradian" ||
          sysname == "boost::units::gradian" ||
          sysname == "angle::gradian")
    {
      os <<"#include <boost/units/systems/angle/gradians.hpp>" <<std::endl;
    }
  else if(sysname == "revolution" ||
          sysname == "boost::units::revolution" ||
          sysname == "angle::revolution")
    {
      os <<"#include <boost/units/systems/angle/revolutions.hpp>" <<std::endl;
    }
  else
    {
      //include necessary non-boost-units system headers
      std::string sysname_sub = boost::replace_all_copy(sysname, "::", "/");
      os <<"#include \"" <<sysname_sub <<".hpp\"" <<std::endl;
    }
}

// Generate necessary units systems headers for an individually defined base unit.
// Unit must be available in this list:
// http://www.boost.org/doc/libs/1_57_0/doc/html/boost_units/Reference.html#boost_units.Reference.alphabetical_listing_of_base_units
inline void include_base_unit_headers(const std::string& base_unit_category_and_name, std::ostream& os){
  os << std::endl;
  std::string cat_name_sub = boost::replace_all_copy(base_unit_category_and_name, "::", "/");
  os <<"#include <boost/units/base_units/" <<cat_name_sub <<".hpp>" <<std::endl;
}


// Generate a unit typedef when given derived_ or base_dimensions
inline void construct_units_typedef_from_dimension(const std::string& fieldname, const std::string& sysname, const bool& absolute, std::ostream& os){

  // Namespace the sysname if necessary
  std::string sysname_ns;
  if(sysname == "si" || sysname == "cgs" || sysname == "celsius" || sysname == "fahrenheit" || sysname == "degree" || sysname == "gradian" || sysname == "revolution")
    sysname_ns = "boost::units::"+sysname+"::system";
  else if(sysname == "boost::units::si" || sysname == "boost::units::cgs")
    sysname_ns = sysname+"::system";
  else if(sysname == "temperature::celsius" || sysname == "temperature::fahrenheit")
    sysname_ns = boost::replace_all_copy(sysname, "temperature::", "boost::units::");
  else if(sysname == "angle::degree" || sysname == "angle::gradian" || sysname == "angle::revolution")
    sysname_ns = boost::replace_all_copy(sysname, "angle::", "boost::units::") + "::system";
  else if(sysname == "angle::radian")
    sysname_ns = "boost::units::si::system";
  else
    sysname_ns = sysname;

  // Typedef the unit
  if(absolute)
    os <<"typedef boost::units::absolute<boost::units::unit<" <<fieldname <<"_dimension," <<sysname_ns <<"> > " <<fieldname <<"_unit;" <<std::endl;
  else // relative temperature or not a temperature (default)
    os <<"typedef boost::units::unit<" <<fieldname <<"_dimension," <<sysname_ns <<"> " <<fieldname <<"_unit;" <<std::endl; 

  os << std::endl;
}

// Generate a dimension typedef when given base_dimensions
inline void construct_base_dims_typedef(const std::vector<std::string>& dim_vec, const std::vector<double>& power_vec, const std::string& fieldname, const std::string& sysname, const bool& rel_temperature, std::ostream& os){
  /////////// BASE DIMENSIONS --> DERIVED FIELD DIMENSION
  bool temperature_dimension = false;
  if(dim_vec[0] == "temperature" && dim_vec.size() == 1)
      temperature_dimension = true;
  if(dim_vec[0] == "dimensionless" && dim_vec.size() == 1)
    {
      os <<"typedef boost::units::dimensionless_type " <<fieldname <<"_dimension;" <<std::endl;;
    }
  else
    {
      os <<"typedef boost::units::derived_dimension< ";
      for(int i=0; i<dim_vec.size(); i++){
	os <<"boost::units::" <<dim_vec[i] <<"_base_dimension," <<power_vec[i] ;
	if(i != dim_vec.size()-1)
	  os <<", ";
      }
      os <<" >::type " <<fieldname <<"_dimension;" <<std::endl;
     }
  os << std::endl;

  construct_units_typedef_from_dimension(fieldname, sysname, 
					 temperature_dimension && !rel_temperature, 
					 os);
}

// Generate a dimension typedef when given derived_dimensions
inline void construct_derived_dims_typedef(const std::vector<std::string>& dim_vec, const std::vector<std::string>& operator_vec, const std::string& fieldname, const std::string& sysname, const bool& rel_temperature, std::ostream& os){
  /////////// DERIVED DIMENSIONS --> DERIVED FIELD DIMENSION

  bool temperature_dimension = false;

  if (dim_vec.size() == 1){
    if(dim_vec[0] == "temperature")
      temperature_dimension = true;
    if(dim_vec[0] == "dimensionless")
      os <<"typedef boost::units::dimensionless_type " <<fieldname <<"_dimension;" <<std::endl;
    else
      os <<"typedef boost::units::" <<dim_vec[0] <<"_dimension " <<fieldname <<"_dimension;" <<std::endl;
  }
  else{//construct new dimension type with mpl divides/times calls based on operators and powers
    //see example here:
    //http://www.boost.org/doc/libs/1_57_0/doc/html/boost_units/Examples.html#boost_units.Examples.DimensionExample
    os <<"typedef ";
    std::string result = dim_vec[0];
    for(int i=0; i<operator_vec.size(); i++){
      if(operator_vec[i] == "/")
        result = "boost::mpl::divides<boost::units::" + result + "_dimension,boost::units::" + dim_vec[i+1] + "_dimension>::type";
      else if(operator_vec[i] == "*")
        result = "boost::mpl::times<boost::units::" + result + "_dimension,boost::units::" + dim_vec[i+1] + "_dimension>::type";
    }
    os <<result <<" " <<fieldname <<"_dimension;" <<std::endl;
  }
  os << std::endl;

  construct_units_typedef_from_dimension(fieldname, sysname, 
					 temperature_dimension && !rel_temperature,
					 os);
}



//=============VVVV


//===========
// Generate a unit typedef when given base_unit
inline void construct_units_typedef_from_base_unit(const std::string& fieldname, const std::string& base_unit_category_and_name, const bool& rel_temperature, std::ostream& os){

  bool temperature_unit = false;

  //expect to see "temperature::celsius" or "temperature::fahrenheit" or "si::kelvin"
  if((base_unit_category_and_name.find("temperature") != std::string::npos) || (base_unit_category_and_name.find("kelvin") != std::string::npos))
    temperature_unit = true;

  bool absolute = temperature_unit && !rel_temperature;

  // Namespace and typedef the unit 
  if(absolute)
    os <<"typedef boost::units::absolute<boost::units::" <<base_unit_category_and_name <<"_base_unit::unit_type> " <<fieldname <<"_unit;" <<std::endl;
  else // relative temperature or not a temperature (default)
    os <<"typedef boost::units::" <<base_unit_category_and_name <<"_base_unit::unit_type " <<fieldname <<"_unit;" <<std::endl;

  os << std::endl;
}

//===========
// Generate the body of the units plugin for the message class
inline void construct_field_class_plugin(const std::string& fieldname, std::ostream& os, const std::string& value_type, bool is_repeated){
  /////////// DERIVED FIELD DIMENSION --> PROTOBUF EXTENSIONS

  // Overloading set_fieldname to accept boost units, i.e. quantity<unit<fieldname_dimension,sysname::system> >
  os <<"template<typename Quantity >" <<std::endl;
  os <<"  void set_" <<fieldname <<"_with_units(";
  if(is_repeated)
      os << "int index, ";
  os <<"Quantity value_w_units)" <<std::endl;
  os <<"  { set_" <<fieldname <<"(";
  if(is_repeated)
      os << "index, ";
  os <<"boost::units::quantity<" <<fieldname <<"_unit," <<value_type <<" >(value_w_units).value() ); };" <<std::endl;
  os << std::endl;

  if(is_repeated)
  {
      os <<"template<typename Quantity >" <<std::endl;
      os <<"  void add_" <<fieldname <<"_with_units(Quantity value_w_units)" <<std::endl;
      os <<"  { add_" <<fieldname <<"(boost::units::quantity<" <<fieldname <<"_unit," <<value_type <<" >(value_w_units).value() ); };" <<std::endl;
      os << std::endl;      
  }

  
  //returns whatever units are requested
  os <<"template<typename Quantity >" <<std::endl;
  os <<"  Quantity " <<fieldname <<"_with_units(";
  if(is_repeated)
      os << "int index";
  os <<") const" <<std::endl;
  os <<"  { return Quantity(" <<fieldname <<"(";
  if(is_repeated)
      os << "index";
  os <<") * " <<fieldname <<"_unit()); };" <<std::endl;
  os << std::endl;

  //returns syname units only
  os <<"boost::units::quantity< " <<fieldname <<"_unit > " <<fieldname <<"_with_units(";
  if(is_repeated)
      os << "int index";
  os <<") const" <<std::endl;
  os <<"  { return " <<fieldname <<"_with_units<boost::units::quantity< " <<fieldname <<"_unit," <<value_type <<" > >(";
  if(is_repeated)
      os << "index";
  os << "); };" <<std::endl;
  os << std::endl;

}

//=============^^^^


#endif 
