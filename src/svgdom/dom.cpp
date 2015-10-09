#include "dom.hpp"

#include <pugixml.hpp>

#include <map>

using namespace svgdom;


namespace{

enum class EXmlNamespace{
	UNKNOWN,
	SVG,
	XLINK
};

const char* DSvgNamespace = "http://www.w3.org/2000/svg";
const char* DXlinkNamespace = "http://www.w3.org/1999/xlink";


struct Parser{
	typedef std::map<std::string, EXmlNamespace> T_NamespaceMap;
	std::vector<T_NamespaceMap> namespaces;
	
	std::vector<EXmlNamespace> defaultNamespace;
	
	
	EXmlNamespace findNamespace(const std::string& ns){
		for(auto i = this->namespaces.rbegin(); i != this->namespaces.rend(); ++i){
			auto iter = i->find(ns);
			if(iter == i->end()){
				continue;
			}
			ASSERT(ns == iter->first)
			return iter->second;
		}
		return EXmlNamespace::UNKNOWN;
	}
	
	struct NamespaceNamePair{
		EXmlNamespace ns;
		std::string name;
	};
	
	NamespaceNamePair getNamespace(const std::string& fullName){
		NamespaceNamePair ret;
		
		auto colonIndex = fullName.find_first_not_of(':');
		if(colonIndex == fullName.length()){
			ret.ns = this->defaultNamespace.back();
			ret.name = fullName;
			return ret;
		}
		
		ASSERT(fullName.length() >= colonIndex + 1)
		
		ret.ns = this->findNamespace(fullName.substr(0, colonIndex));
		ret.name = fullName.substr(colonIndex + 1, fullName.length() - 1 - colonIndex);
		
		return ret;
	}
	
	std::unique_ptr<svgdom::Element> parseNode(const pugi::xml_node& n);

	std::unique_ptr<SvgElement> parseSvgElement(const pugi::xml_node& n){
		//TODO:
		return nullptr;
	}
};//~class


std::unique_ptr<svgdom::Element> Parser::parseNode(const pugi::xml_node& n){
	//parse default namespace
	{
		pugi::xml_attribute dn = n.attribute("xmlns");
		if(!dn.empty()){
			if(std::string(dn.value()) == DSvgNamespace){
				this->defaultNamespace.push_back(EXmlNamespace::SVG);
			}else{
				this->defaultNamespace.push_back(EXmlNamespace::UNKNOWN);
			}
		}else{
			this->defaultNamespace.push_back(this->defaultNamespace.back());
		}
	}
	
	//parse other namespaces
	{
		std::string xmlns = "xmlns:";
		
		this->namespaces.push_back(T_NamespaceMap());
		
		for(auto a = n.first_attribute(); !a.empty(); a = a.next_attribute()){
			auto attr = std::string(a.name());
			
			if(attr.substr(0, xmlns.length()) != xmlns){
				continue;
			}
			
			ASSERT(attr.length() >= xmlns.length())
			auto ns = attr.substr(xmlns.length(), attr.length() - xmlns.length());
			
			if(ns == DSvgNamespace){
				this->namespaces.back()[ns] = EXmlNamespace::SVG;
			}else if (ns == DXlinkNamespace){
				this->namespaces.back()[ns] = EXmlNamespace::XLINK;
			}else{
				this->namespaces.back().erase(ns);
			}
		}
	}
	
	auto nsn = getNamespace(n.value());
	switch(nsn.ns){
		case EXmlNamespace::SVG:
			if(nsn.name == "svg"){
				return parseSvgElement(n);
			}
			break;
		default:
			//unknown namespace, ignore
			break;
	}
	
	ASSERT(this->namespaces.size() > 0)
	this->namespaces.pop_back();
	ASSERT(this->defaultNamespace.size() > 0)
	this->defaultNamespace.pop_back();
	
	return nullptr;
}

}//~namespace



std::unique_ptr<SvgElement> svgdom::load(const papki::File& f){
	pugi::xml_document doc;
	{
		auto fileContents = f.loadWholeFileIntoMemory();
		if(doc.load_buffer(&*fileContents.begin(), fileContents.size()).status != pugi::xml_parse_status::status_ok){
			TRACE(<< "svgdom::load(): loading XML document failed!" << std::endl)
			return nullptr;
		}
	}
	
	Parser parser;
	
	auto ret = parser.parseNode(doc.first_child());
	
	return std::unique_ptr<SvgElement>(dynamic_cast<SvgElement*>(ret.release()));
}