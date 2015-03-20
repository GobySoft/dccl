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

namespace client
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
      bool parse_base_dimensions(Iterator first, Iterator last, std::vector<double>& v, std::vector<std::string>& vc, std::vector<std::string>& vs)
  {
    using qi::double_;
    using qi::char_;
    using qi::_1;
    using qi::phrase_parse;
    using ascii::space;
    using phoenix::ref;
    using phoenix::bind; 
    using phoenix::push_back;
    using qi::eps;

    try 
      {

	bool r = phrase_parse(
			      first,                          /*< start iterator >*/
			      last,                           /*< end iterator >*/
			      +((char_("LTMASIKNJB-")[bind(&push_char_base, ref(vc), ref(vs), _1)] | 
				 ((ascii::string("length") | ascii::string("time") | ascii::string("mass") | ascii::string("plane_angle") | ascii::string("solid_angle") | ascii::string("current") | ascii::string("temperature") | ascii::string("amount") | ascii::string("luminous_intensity") | ascii::string("information") | ascii::string("dimensionless"))[bind(&push_string_base, ref(vc), ref(vs), _1)])) >>
				-(ascii::string("_base_dimension")) >>
				('^' > double_[push_back(ref(v), _1)] | eps[push_back(ref(v), 1)])),
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
      bool parse_derived_dimensions(Iterator first, Iterator last, std::vector<std::string>& vs_op, std::vector<std::string>& vs_derived)
  {
    using qi::double_;
    using qi::char_;
    using qi::_1;
    using qi::phrase_parse;
    using ascii::space;
    using phoenix::ref;
    using phoenix::bind; 
    using phoenix::push_back;
    using qi::eps;

    try 
      {
	std::vector<std::string> params;
	bool r = boost::spirit::qi::parse(first, last,
					  +((+char_("a-z1_"))[boost::phoenix::bind(&push_char_vec, boost::phoenix::ref(params), _1)] >>
					    -(*char_(" ") >> (char_("*/")[bind(&push_char, ref(vs_op), _1)] | eps[push_back(ref(vs_op), "*")]) >> *char_(" "))));

	if(vs_op.size())
	  vs_op.pop_back();
    
	for(std::vector<std::string>::iterator it = params.begin(), end = params.end(); it!=end; ++it)
	  {
	    std::string::size_type dim_pos = it->find("_dimension");
	    if(dim_pos != std::string::npos)
	      *it = it->substr(0, dim_pos);
	    //std::cout << *it << std::endl;
	    vs_derived.push_back(*it);
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

  if(sysname == "si" ||
     sysname == "boost::units::si" ||
     sysname == "boost::units::si::system")
  {
      os <<"#include <boost/units/systems/si.hpp>" <<std::endl;
  }
  else if(sysname == "cgs" ||
          sysname == "boost::units::cgs" ||
          sysname == "boost::units::cgs::system")
  {
      os <<"#include <boost/units/systems/cgs.hpp>" <<std::endl;
  }
  else
  {
      //include necessary non-boost-units system headers
      std::string sysname_sub = boost::replace_all_copy(sysname, "::", "/");
      os <<"#include \"" <<sysname_sub <<".hpp\"" <<std::endl;
  }
}

// Generate the beginning (auto-generated) piece of the protobuf message's class (for testing)
inline void gen_basic_class_head(const bool qty_is_integer, const std::string& ClassName, const std::string& fieldname, std::ostream& os){

  os <<"class " <<ClassName <<std::endl;
  os <<"{" <<std::endl;
  os <<" public:" <<std::endl;
  os <<std::endl;
  if(qty_is_integer)
    {
      os <<"  void set_" <<fieldname <<"(int value)" <<std::endl;
      os <<"  { value_ = value; };" <<std::endl;
      os <<std::endl;
      os <<"  int " <<fieldname <<"() const" <<std::endl;
      os <<"  { return value_; };" <<std::endl;
    }
  else
    {// qty is double
      os <<"  void set_" <<fieldname <<"(double value)" <<std::endl;
      os <<"  { value_ = value; };" <<std::endl;
      os <<std::endl;
      os <<"  double " <<fieldname <<"() const" <<std::endl;
      os <<"  { return value_; };" <<std::endl;
    }
  os <<std::endl;
}


///GENERATE THIS PART vvvvvvvvvvvvvvv

// Generate a dimension typedef when given base_dimensions
inline void handle_base_dims(const std::vector<std::string>& dim_vec, const std::vector<double>& power_vec, const std::string& fieldname, std::ostream& os){
  /////////// BASE DIMENSIONS --> DERIVED FIELD DIMENSION
  os <<"typedef boost::units::derived_dimension< ";
  for(int i=0; i<dim_vec.size(); i++){
    os <<"boost::units::" <<dim_vec[i] <<"_base_dimension," <<power_vec[i] ;
    if(i != dim_vec.size()-1)
      os <<", ";
  }
  os <<" >::type " <<fieldname <<"_dimension;" <<std::endl;
  os << std::endl;
}

// Generate a dimension typedef when given derived_dimensions
inline void handle_derived_dims(const std::vector<std::string>& dim_vec, const std::vector<std::string>& operator_vec, const std::string& fieldname, std::ostream& os){
  /////////// DERIVED DIMENSIONS --> DERIVED FIELD DIMENSION
  if (dim_vec.size() == 1){
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
}

// Generate the body of the units plugin for the message class
inline void construct_field_class_plugin(const bool qty_is_integer, const std::string& fieldname, const std::string& sysname, std::ostream& os){
  /////////// DERIVED FIELD DIMENSION --> PROTOBUF EXTENSIONS

  // Namespace the sysname if necessary
  std::string sysname_ns;
  if(sysname == "si" || sysname == "cgs")
    sysname_ns = "boost::units::"+sysname+"::system";
  else if(sysname == "boost::units::si" || sysname == "boost::units::cgs")
    sysname_ns = sysname+"::system";
  else
    sysname_ns = sysname;

  // Overloading set_fieldname to accept boost units, i.e. quantity<unit<fieldname_dimension,sysname::system> >
  os <<"template<typename Quantity >" <<std::endl;
  os <<"  void set_" <<fieldname <<"_with_units(Quantity value_w_units)" <<std::endl;
  if(qty_is_integer)
    {
      os <<"  { set_" <<fieldname <<"(round(boost::units::quantity<boost::units::unit<" <<fieldname <<"_dimension," <<sysname_ns <<"> >(value_w_units) / boost::units::unit<" <<fieldname <<"_dimension," <<sysname_ns <<">() )); };" <<std::endl;
    }
  else
    {// don't round if double
      os <<"  { set_" <<fieldname <<"(boost::units::quantity<boost::units::unit<" <<fieldname <<"_dimension," <<sysname_ns <<"> >(value_w_units) / boost::units::unit<" <<fieldname <<"_dimension," <<sysname_ns <<">() ); };" <<std::endl;
    }
  os << std::endl;
  
  //returns whatever units are requested
  os <<"template<typename Quantity >" <<std::endl;
  os <<"  Quantity " <<fieldname <<"_with_units() const" <<std::endl;
  os <<"  { return Quantity(" <<fieldname <<"() * boost::units::unit<" <<fieldname <<"_dimension," <<sysname_ns <<">()); };" <<std::endl;
  os << std::endl;

  //returns syname units only
  os <<"boost::units::quantity< boost::units::unit<" <<fieldname <<"_dimension," <<sysname_ns <<"> > " <<fieldname <<"_with_units() const" <<std::endl;
  os <<"  { return " <<fieldname <<"_with_units<boost::units::quantity< boost::units::unit<" <<fieldname <<"_dimension," <<sysname_ns <<"> > >(); };" <<std::endl;
  os << std::endl;
}

///GENERATE THIS PART ^^^^^^^^^^^^^^

// Generate the ending (auto-generated) piece of the protobuf message's class (for testing)
inline void gen_basic_class_foot(const bool qty_is_integer, std::ostream& os){
  os <<" private:" <<std::endl;
  os <<std::endl;
  if(qty_is_integer)
    {
      os <<"  int value_;" <<std::endl;
    }
  else
    {//qty is double
      os <<"  double value_;" <<std::endl;
    }
  os <<std::endl;
  os <<"};" <<std::endl;
  os <<std::endl;

  /* os <<"//===================" <<std::endl; */
  /* os <<std::endl; */
}

#endif 
