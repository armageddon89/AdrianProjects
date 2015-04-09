// CodeAnalisys.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static llvm::cl::extrahelp MoreHelp("\nMore help text...");

int number = 0;

namespace Parser
{
	enum IPCMethodCallType
	{
		NONE,
		POST_MESSAGE,
		SEND_MESSAGE,
		SUBSCRIBE_PROCESS,
		SUBSCRIBE_DOMAIN,
		VARIANT_CODING
	};
};

std::string currentProcess;
std::string clangPath;
std::set<std::string> filesParsed;
std::multimap<std::string, std::vector<std::string>> functionsMapping;
std::map<std::pair<std::string, std::string>, std::set<std::string>> componentsDependencies;
std::map<std::string, std::string> processCppMap;
std::map<std::string, std::vector<std::string>> cppHeadersMapping;

class FindNamedClassVisitor
	: public clang::RecursiveASTVisitor<FindNamedClassVisitor> {
public:
	explicit FindNamedClassVisitor(clang::ASTContext *Context)
		: Context(Context) {}

	virtual bool VisitCXXConstructExpr(clang::CXXConstructExpr *expr)
	{
		auto type = expr->getType()->getCanonicalTypeInternal().getAsString();
		auto fileName = Context->getSourceManager().getFilename(expr->getLocStart()).str();

		if (type == "class CIPCCommunicator")
		{
			for (auto child : expr->children())
			{
				auto name = child->getStmtClassName();
				if (name == std::string("DeclRefExpr"))
				{
					clang::DeclRefExpr *refExpr = static_cast<clang::DeclRefExpr *>(child);
					currentProcess = refExpr->getNameInfo().getAsString();
					processCppMap.insert(std::make_pair(currentProcess, fileName));
					break;
				}
			}
		}

		return true;
	}

	virtual bool VisitCXXMemberCallExpr(clang::CXXMemberCallExpr *expr)
	{
		auto subexpr = expr->getRawSubExprs();
		number++;
	
		Parser::IPCMethodCallType type = Parser::NONE;
		std::vector<std::string> params;
		std::string vcCallName;

		for (auto it : subexpr)
		{
			std::string className = it->getStmtClassName();

			if (className == "MemberExpr")
			{
				clang::MemberExpr *expression = static_cast<clang::MemberExpr *>(it);
				auto nameInfo = expression->getMemberNameInfo();
				std::string info = nameInfo.getAsString();
				if (info == "postMessage")
				{
					type = Parser::POST_MESSAGE;
				}
				else if (info == "sendMessage")
				{
					type = Parser::SEND_MESSAGE;
				}
				else if (info == "subscribeToEvent")
				{
					type = Parser::SUBSCRIBE_PROCESS;
				}
				else if (info == "subscribeToDomain")
				{
					type = Parser::SUBSCRIBE_DOMAIN;
				}
				else if ((info.find("set") == 0 || info.find("get") == 0) && info != "getCommLayer")
				{
					type = Parser::VARIANT_CODING;
					vcCallName = info;
				}
			}

			if (type != Parser::NONE)
			{
				bool bFound = false;

				for (auto child : it->children())
				{
					std::string str = child->getStmtClassName();
					if (str == "MemberExpr" || str == "CXXThisExpr" || str == "ImplicitCastExpr" || str == "DeclRefExpr")
					{
						clang::MemberExpr *memExpr = (clang::MemberExpr *)(child);
						std::string typeName = memExpr->getType().getAsString();
						
						if (type == Parser::VARIANT_CODING &&
							(typeName.find("NVariantCoding::CVariantCodingAccessorRead") != std::string::npos))
						{
							type = Parser::VARIANT_CODING;
							bFound = true;
							break;
						}
						else 
						if (type != Parser::VARIANT_CODING &&
							(typeName == "class CIPCCommunicator" || typeName == "class CIPCCommunicator *"))
						{
							bFound = true;
							break;
						}
						else
						{
							type = Parser::NONE;
							break;
						}
					}
				}

				if (!bFound)
				{
					type = Parser::NONE;
				}

				break;
			}
		}

		if (type != Parser::NONE)
		{
			auto fileName = Context->getSourceManager().getFilename(expr->getLocStart()).str();

			if (type == Parser::POST_MESSAGE)
			{
				params.push_back("postMessage");
			}
			else if (type == Parser::SEND_MESSAGE)
			{
				params.push_back("sendMessage");
			}
			else if (type == Parser::SUBSCRIBE_DOMAIN || type == Parser::SUBSCRIBE_PROCESS)
			{
				params.push_back("subscribe");
			}
			else if (type == Parser::VARIANT_CODING)
			{
				if (fileName.find("CVariantCodingAccessorRead") == std::string::npos
					&& fileName.find("CVariantCodingAccessorWrite") == std::string::npos)
				{
					params.push_back("variantCoding");
					params.push_back(vcCallName);
				}
			}

			for (auto arg : expr->arguments())
			{
				std::string className = arg->getStmtClassName();

				if (className == "DeclRefExpr")
				{
					clang::DeclRefExpr *declRef = static_cast<clang::DeclRefExpr *>(arg);
					std::string declName = declRef->getType()->getCanonicalTypeInternal().getAsString();
					params.push_back(declName);
					params.push_back(declRef->getNameInfo().getAsString());
				}
				else if (className == "IntegerLiteral")
				{
					clang::IntegerLiteral *integralValue = static_cast<clang::IntegerLiteral *>(arg);
					int value = static_cast<int>(integralValue->getValue().getSExtValue());

					std::stringstream stream;
					stream << value;
					params.push_back(stream.str());
				}
				else if (className == "MemberExpr")
				{
					clang::MemberExpr *memberExpr = static_cast<clang::MemberExpr *>(arg);
					std::string name = memberExpr->getMemberNameInfo().getAsString();
					std::string type = memberExpr->getType()->getCanonicalTypeInternal().getAsString();
					params.push_back(type);
					params.push_back(name);
				}
				else if (className == "ImplicitCastExpr")
				{
					clang::ExplicitCastExpr *explicitCast = static_cast<clang::ExplicitCastExpr *>(arg);
					std::string type = explicitCast->getSubExpr()->getType().getAsString();
					if (type.find("enum") == 0)
					{
						clang::DeclRefExpr *enumClass = static_cast<clang::DeclRefExpr *>(explicitCast->getSubExpr());
						std::string typeKind = enumClass->getNameInfo().getAsString();
						params.push_back(type);
						params.push_back(typeKind);
					}
				}
			}

			functionsMapping.insert(std::make_pair(fileName, params));
		}
		
		return true;
	}

private:
	clang::ASTContext *Context;
};

class ExampleASTConsumer : public clang::ASTConsumer {
private:
	FindNamedClassVisitor *visitor; // doesn't have to be private

public:
	// override the constructor in order to pass CI
	explicit ExampleASTConsumer(clang::CompilerInstance *CI)
		: visitor(new FindNamedClassVisitor(&CI->getASTContext())) // initialize the visitor
	{ }

	// override this to call our ExampleVisitor on each top-level Decl
	virtual bool HandleTopLevelDecl(clang::DeclGroupRef DG) {
		// a DeclGroupRef may have multiple Decls, so we iterate through each one
		for (clang::DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; i++) {
			clang::Decl *D = *i;
			visitor->TraverseDecl(D); // recursively visit each AST node in Decl "D"
		}
		return true;
	}
};

class ExampleFrontendAction : public clang::ASTFrontendAction {
public:
	virtual clang::ASTConsumer *CreateASTConsumer(clang::CompilerInstance &CI, clang::StringRef file) {
		return new ExampleASTConsumer(&CI); // pass CI pointer to ASTConsumer
	}
};

void launchClangTool(const std::string &cppFile, const std::vector<std::string> &tokens)
{
	int nrArgs = 5;
	int argc = tokens.size() + nrArgs;

	char **argv = nullptr;

	argv = new char *[argc];
	std::string toolName = "CodeAnalisys.exe";
	std::string delimiter = "--";
	std::string target = "-target";
	std::string targetName = "arm-linux-androideabi";
	std::string compiler = "-std=c++11";

	argv[0] = new char[toolName.size() + 1];
	argv[1] = new char[cppFile.size() + 1];
	argv[2] = new char[delimiter.size() + 1];
	argv[3] = new char[target.size() + 1];
	argv[4] = new char[targetName.size() + 1];
	argv[5] = new char[compiler.size() + 1];

	memcpy(argv[0], toolName.c_str(), toolName.size() + 1);
	memcpy(argv[1], cppFile.c_str(), cppFile.size() + 1);
	memcpy(argv[2], delimiter.c_str(), delimiter.size() + 1);
	memcpy(argv[3], target.c_str(), target.size() + 1);
	memcpy(argv[4], targetName.c_str(), targetName.size() + 1);
	memcpy(argv[5], compiler.c_str(), compiler.size() + 1);
	
	for (int i = 0; i < tokens.size(); i++)
	{
		argv[i + nrArgs] = new char[tokens[i].size() + 1];
		memcpy(argv[i + nrArgs], tokens[i].c_str(), tokens[i].size() + 1);
	}

	/*
	for (int i = 0; i < argc; i++)
	{
		std::cout << argv[i] << " ";
	}
	std::cout << std::endl;
	*/
	//std::cout << "##- " << cppFile << std::endl;

	clang::tooling::CommonOptionsParser OptionsParser(argc, const_cast<const char **>(argv), MyToolCategory);
	std::vector<std::string> currentFiles;
	currentFiles.push_back(cppFile);

	std::string headersCommand = clangPath + " " + "-MD -MF headers.d ";
	headersCommand.append(cppFile);
	headersCommand.append(" ");
	headersCommand.append(target + " ");
	headersCommand.append(targetName + " ");
	headersCommand.append("-c ");
	for (int i = 0; i < tokens.size(); i++)
	{
		headersCommand.append(tokens[i] + " ");
	}

	system(headersCommand.c_str()); 

	char buffer[1024];
	std::ifstream file("headers.d");
	file.getline(buffer, sizeof(buffer));
	file.getline(buffer, sizeof(buffer));

	std::vector<std::string> headers;
	while (!file.eof())
	{
		file.getline(buffer, sizeof(buffer));
		std::istringstream fin(buffer);
		std::string token;
		fin >> token;

		std::replace(token.begin(), token.end(), '/', '\\');

		headers.push_back(token);
	}

	cppHeadersMapping.insert(std::make_pair(cppFile, headers));

	clang::tooling::ClangTool Tool(OptionsParser.getCompilations(),
								   cppFile);
	
	auto fac = clang::tooling::newFrontendActionFactory<ExampleFrontendAction>();
	Tool.run(fac.get());

	//std::cout << "current process is " << currentProcess << std::endl;
	currentProcess = "";
	/*
	if (cppFile.find("Proc1.cpp") != std::string::npos)
	{
		auto fac = clang::tooling::newFrontendActionFactory<clang::ASTDumpAction>();
		Tool.run(fac.get());
	}*/
}

void extractComponents(std::string projectPath, std::vector<std::string> &components, std::vector<std::string> &cppFiles)
{
	using namespace std::tr2::sys;

	for (auto it = recursive_directory_iterator(projectPath); it != recursive_directory_iterator(); it++)
	{
		auto path = it->path();
		if (is_directory(path))
		{
			bool bApi = false;
			bool bSrc = false;
			bool bTest = false;
			bool bDoc = false;
			for (auto it = directory_iterator(path); it != directory_iterator(); it++)
			{
				auto path = it->path();
				if (!is_directory(path))
				{
					continue;
				}

				if (path.filename() == "api")
				{
					bApi = true;
				}
				else if (path.filename() == "src")
				{
					bSrc = true;
				}
				else if (path.filename() == "test")
				{ 
					bTest = true;
				}
				else if (path.filename() == "doc")
				{
					bDoc = true;
				}
			}

			int numberOfCompFolders = 0;
			if (bApi) numberOfCompFolders++;
			if (bSrc) numberOfCompFolders++;
			if (bTest) numberOfCompFolders++;
			if (bDoc) numberOfCompFolders++;
			if (numberOfCompFolders >= 3)
			{
				components.push_back(path.file_string());
			}
		}
		else
		{
			if (path.extension() == ".cpp")
			{
				cppFiles.push_back(path.file_string());
			}
		}
	}
}

void printLibrariesMap(const std::map<std::string, std::vector<std::string>> &libraries,
					   const std::map<std::string, std::vector<std::string>> &objects)
{
	for (auto it : libraries)
	{
		std::cout << it.first << std::endl;
		for (auto it2 : it.second)
		{
			std::cout << "\t" << it2 << std::endl;
			if (it2.back() == 'a')
			{
				auto objs = objects.find(it2);
				if (objs != objects.end())
				{
					for (auto obj : objs->second)
					{
						std::cout << "\t\t" << obj << std::endl;
					}
				}
			}
		}
	}
}

enum NodeType
{
	NodeExe,
	NodeLib,
	NodeSo,
	NodeCpp,
	NodeRoot
};

struct Node
{
	NodeType type;
	std::vector<Node> children;
	std::string name;
};

Node constructTree(const std::map<std::string, std::vector<std::string>> &libraries,
				   const std::map<std::string, std::vector<std::string>> &objects)
{
	Node root;
	root.type = NodeRoot;

	auto fnConstructSo = [&](std::string binName)
	{
		Node soNode;
		soNode.type = NodeSo;
		soNode.name = binName;
		auto foundSo = libraries.find(binName);
		if (foundSo != libraries.end())
		{
			for (auto it : foundSo->second)
			{
				Node subNode;
				if (it.substr(it.size() - 3, 3) == ".so")
				{
					subNode.type = NodeSo;
				}
				else if (it.substr(it.size() - 4, 4) == ".cpp")
				{
					subNode.type = NodeCpp;
				}
				subNode.name = it;
				soNode.children.push_back(subNode);
			}
		}

		return soNode;
	};

	auto fnConstructA = [&](std::string aName)
	{
		Node aNode;
		aNode.type = NodeLib;
		aNode.name = aName;

		auto found = objects.find(aName);
		if (found != objects.end())
		{
			for (auto it : found->second)
			{
				Node subNode;
				subNode.type = NodeCpp;
				subNode.name = it;

				aNode.children.push_back(subNode);
			}
		}

		return aNode;
	};

	for (auto it : libraries)
	{
		auto binName = it.first;
		Node node;

		if (binName.substr(binName.size() - 3, 3) == ".so")
		{
			node = fnConstructSo(binName);
		}
		else
		{
			node.name = binName;
			for (auto file : it.second)
			{
				Node subNode;
				if (file.substr(file.size() - 3, 3) == ".so")
				{
					subNode = fnConstructSo(file);
				}
				else if (file.substr(file.size() - 4, 4) == ".cpp")
				{
					subNode.type = NodeCpp;
					subNode.name = file;
				}
				else if (file.substr(file.size() - 2, 2) == ".a")
				{
					subNode = fnConstructA(file);
				}

				node.children.push_back(subNode);
			}
		}

		root.children.push_back(node);
	}

	return root;
}

void printTree(Node node, int level)
{
	static std::ofstream printer;
	auto slash = node.name.find_last_of("\\");
	std::string name = node.name;
	if (slash != std::string::npos)
	{
		name = node.name.substr(slash + 1);
	}

	if (level == 1)
	{
		printer.close();
		printer.open(name + ".dot");
		printer << "digraph\n{\n";
	}

	for (auto it : node.children)
	{
		if (level > 0)
		{
			printer << "\"" << name << "\" -> \"" << it.name.substr(it.name.find_last_of("\\") + 1) << "\"" << std::endl;
		}
		printTree(it, level + 1);
	}

	if (level == 1)
	{
		printer << "}" << std::endl;
	}
}

std::multimap<std::string, std::string> mapCppToProcess(Node node, std::map<std::string, std::string> processCppMap)
{
	std::multimap<std::string, std::string> cppProcessMap;

	std::function<bool(Node, std::string)> fnFind = [&](Node node, std::string cppFile)
	{
		bool bRet = false;

		for (auto it : node.children)
		{
			if (it.type != NodeCpp)
			{
				bRet = fnFind(it, cppFile);
				if (bRet == true)
				{
					return bRet;
				}
			}

			if (it.name == cppFile)
			{
				return true;
			}
		}

		return false;
	};

	std::function<void(Node, std::string)> fnFill = [&](Node node, std::string fileName)
	{
		if (node.type == NodeCpp)
		{
			cppProcessMap.insert(std::make_pair(node.name, fileName));
			return;
		}

		for (auto it : node.children)
		{
			fnFill(it, fileName);
		}
	};

	for (auto it : processCppMap)
	{
		for (auto it1 : node.children)
		{
			if (fnFind(it1, it.second))
			{
				fnFill(it1, it.first);
			}
		}
	}

	return cppProcessMap;
}

struct MessageExchange
{
	std::set<std::string> send;
	std::set<std::string> respond;
	std::set<std::string> post;
	std::set<std::string> subscribe;
};

int main(int argc, const char **argv) 
{
	//freopen("E:\dumpFile.txt", "w", stdout);

	std::string hbblLogPath = argv[1];
	std::string projectPath = argv[2];
	clangPath = argv[3];
	if (projectPath.back() != '\\')
	{
		projectPath.push_back('\\');
	}
	std::string productsDir = projectPath.substr(0, projectPath.size() - 1) + "_products\\";

	std::string impPath = projectPath + "imp";

	std::ifstream fin(hbblLogPath);
	char maxBuf[10000];
	std::string gppName = "arm-linux-androideabi-g++";
	std::string cName = "arm-linux-androideabi-gcc";

	std::map<std::string, std::vector<std::string>> processLibsMap;
	std::map<std::string, std::vector<std::string>> libsObjectsMap;

	/*
	for (auto i : components)
	{
		std::cout << i << std::endl;
	}
	for (auto i : cppNames)
	{
		std::cout << i << std::endl;
	}
	*/

	while (!fin.eof())
	{
		std::vector<std::string> tokens;

		fin.getline(maxBuf, sizeof(maxBuf));

		char *p = maxBuf;
		while (*p == ' ')
		{
			p++;
		}

		if (std::strlen(p) > gppName.size() && !strncmp(gppName.c_str(), p, gppName.size()))
		{
			std::istringstream stream(maxBuf);
			std::string token;
			std::string cpp;
			bool bAlreadyParsed = false;
			while (std::getline(stream, token, ' '))
			{
				if (token.size() > 4 && ((token.substr(token.size() - 4) == ".cpp")))
				{
					cpp = projectPath + token;
					if (filesParsed.find(cpp) == filesParsed.cend())
					{
						filesParsed.insert(cpp);
					}
					else
					{
						bAlreadyParsed = true;
						break;
					}
				}
				else if (token[0] == '-' && token[1] == 'I')
				{
					if (token.find(projectPath) == std::string::npos)
					{
						token.insert(2, projectPath);
					}
					tokens.push_back(token);
				}
			}

			if (!bAlreadyParsed)
			{
				launchClangTool(cpp, tokens);
			}
		}
		else if (std::strlen(p) >= 4 && !strncmp(p, "Link", 4))
		{
			std::istringstream stream(maxBuf);
			std::string execName;
			std::getline(stream, execName, ' ');
			std::getline(stream, execName, ' ');
			fin.getline(maxBuf, sizeof(maxBuf));
			fin.getline(maxBuf, sizeof(maxBuf));
			fin.getline(maxBuf, sizeof(maxBuf));

			stream.clear();
			stream.str(maxBuf);

			std::string token;
			std::vector<std::string> libs;
			std::string keywordInput = "INPUT(";
			while (std::getline(stream, token, ' '))
			{
				if (token.size() > 4 && token.find(".txt") == token.size() - std::strlen(".txt"))
				{
					std::ifstream fin(token);
					if (fin)
					{
						while (!fin.eof())
						{
							fin >> token;
							if (token.size() < 5 || token.find("INPUT(") != 0)
							{
								continue;
							}

							std::string lib = token.substr(keywordInput.size(), token.size() - keywordInput.size() - 1);
							if (lib.find(projectPath) == std::string::npos && 
								lib.find(productsDir) == std::string::npos )
							{
								lib = projectPath + lib;
							}
							if (lib.find("S.o") == lib.size() - 3)
							{
								lib = lib.substr(0, lib.size() - 3) + ".o";
							}

							if (lib.substr(lib.size() - 2, 2) == ".o")
							{
								auto index = lib.find("\\imp") + 1;
								lib = projectPath + lib.substr(index);
								lib.pop_back();
								lib.pop_back();
								lib.append(".cpp");
							}

							libs.push_back(lib);
							token.clear();
						}
					}
				}
			}

			processLibsMap.insert(std::make_pair(execName, libs));
		}
		else if (std::strlen(p) >= 7 && !strncmp(p, "Archive", 7))
		{
			std::istringstream stream(maxBuf);
			std::string libName;
			std::getline(stream, libName, ' ');
			std::getline(stream, libName, ' ');
			fin.getline(maxBuf, sizeof(maxBuf));
			fin.getline(maxBuf, sizeof(maxBuf));
			fin.getline(maxBuf, sizeof(maxBuf));
			fin.getline(maxBuf, sizeof(maxBuf));

			stream.clear();
			stream.str(maxBuf);

			if (!stream)
			{
				continue;
			}

			std::string token;
			std::vector<std::string> objects;
			while (!stream.eof())
			{
				stream >> token;
				if (!token.empty() && token.front() == '\"')
				{
					break;
				}
			}

			token = token.substr(1, token.size() - 2);
			token.append(".t");

			std::ifstream fin(token);
			fin.getline(maxBuf, sizeof(maxBuf));
			std::string object;
			while (!fin.eof())
			{
				fin >> object;
				if (object != "ADDMOD")
				{
					continue;
				}
				fin >> object;
				
				for (int i = 0; i < object.size(); i++)
				{
					if (object[i] == '/')
					{
						object[i] = '\\';
					}
				}

				if (object.substr(object.size() - 2, 2) == ".o")
				{
					auto index = object.find("\\imp") + 1;
					object = projectPath + object.substr(index);
					object.pop_back();
					object.pop_back();
					object.append(".cpp");
				}

				objects.push_back(object);
			}

			libsObjectsMap.insert(std::make_pair(libName, objects));
		}
	}

	auto tree = constructTree(processLibsMap, libsObjectsMap);

	for (auto it : processCppMap)
	{
		std::cout << it.first << " " << it.second;
	}

	auto processMapping = mapCppToProcess(tree, processCppMap);

	for (auto it : processMapping)
	{
		std::cout << it.first << " " << it.second << std::endl;
	}

	std::map<std::pair<std::string, std::string>, MessageExchange> graphMapping;

	std::ofstream vcFile("variantCoding.txt");

	for (auto it : functionsMapping)
	{
		std::string fileName = it.first;
		auto params = it.second;

		auto msgType = params[0];
		std::string typeSend;
		std::string typeReceived;
		std::string messageNameSend;
		std::string messageNameReceived;
		std::string destinationProcess;
		std::string subscribeDomain;
		std::string subscribeType;

		if (msgType == "variantCoding")
		{
			vcFile << fileName << " - ";
			vcFile << params[1] << "(";
			for (int i = 2; i < params.size(); i++)
			{
				vcFile << params[i] << ", ";
			}
			vcFile << ")" << std::endl;
		}
		else
		{
			for (size_t i = 1; i < params.size(); i++)
			{
				if ((params[i].find("class") == 0) || (params[i].find("struct") == 0))
				{
					if (typeSend.empty())
					{
						typeSend = params[i];
						messageNameSend = params[i + 1];
					}
					else
					{
						typeReceived = params[i];
						messageNameReceived = params[i + 1];
					}
					i++;
				}
				else if (params[i] == "enum NIPC::Process")
				{
					destinationProcess = params[i + 1];
					i++;
				}
				else if (params[i] == "enum NIPC::Domain")
				{
					subscribeDomain = params[i + 1];
					if (i + 3 < params.size())
					{
						subscribeType = params[i + 3];
					}
				}
			}

			auto sources = processMapping.equal_range(fileName);

			for (auto it = sources.first; it != sources.second; it++)
			{
				std::string graphDestination = destinationProcess;
				if (destinationProcess.empty())
				{
					graphDestination = "unknown";
				}

				std::string info;

				auto pair = std::make_pair(it->second, graphDestination);
				auto found = graphMapping.find(pair);

				MessageExchange exchange;
				if (found != graphMapping.end())
				{
					exchange = found->second;
				}

				if (msgType == "sendMessage")
				{
					if (!typeSend.empty())
					{
						exchange.send.insert(typeSend);
					}
					if (!typeReceived.empty())
					{
						info = typeReceived;

						auto pair = std::make_pair(graphDestination, it->second);
						auto found = graphMapping.find(pair);
						if (found != graphMapping.end())
						{
							found->second.respond.insert(typeReceived);
						}
						else
						{
							MessageExchange messageExchange;
							messageExchange.respond.insert(typeReceived);
							graphMapping.insert(std::make_pair(pair, messageExchange));
						}
					}
				}
				else if (msgType == "postMessage")
				{
					if (!typeSend.empty())
					{
						exchange.post.insert(typeSend);
					}
				}
				else if (msgType == "subscribe")
				{
					if (!subscribeDomain.empty())
					{
						info.append(subscribeDomain + " ");
					}
					if (!subscribeType.empty())
					{
						info.append(subscribeType);
					}

					exchange.subscribe.insert(info);
				}

				if (found == graphMapping.end())
				{
					graphMapping.insert(std::make_pair(pair, exchange));
				}
				else
				{
					found->second = exchange;
				}
			}
		}
	}

	std::ofstream fout("graph.dot");

	fout << "digraph G" << std::endl << "{" << std::endl;

	for (auto it : graphMapping)
	{
		if (!it.second.send.empty())
		{
			fout << it.first.first << "->" << it.first.second;
			fout << " [ label=\"";
			fout << "send(";
			for (auto it : it.second.send)
			{
				fout << it << ", ";
			}
			fout << ")\" ]";
			fout << std::endl;
		}

		if (!it.second.post.empty())
		{
			fout << it.first.first << "->" << it.first.second;
			fout << " [ label=\"";
			fout << "post(";
			for (auto it : it.second.post)
			{
				fout << it << ", ";
			}
			fout << ")\" ]";
			fout << std::endl;
		}

		if (!it.second.subscribe.empty())
		{
			fout << it.first.first << "->" << it.first.second;
			fout << " [ label=\"";
			fout << "subscribe(";
			for (auto it : it.second.subscribe)
			{
				fout << it << ", ";
			}
			fout << ")\" ]";
			fout << std::endl;
		}

		if (!it.second.respond.empty())
		{
			fout << it.first.first << "->" << it.first.second;
			fout << " [ label=\"";
			fout << "respond(";
			for (auto it : it.second.respond)
			{
				fout << it << ", ";
			}
			fout << ")\" ]";
			fout << std::endl;
		}
	}

	fout << "}" << std::endl;

	std::vector<std::string> components;
	std::vector<std::string> cppNames;
	extractComponents(impPath, components, cppNames);

	for (auto it : cppHeadersMapping)
	{
		auto fileName = it.first;

		for (auto header : it.second)
		{
			if (header.find("\\api") == std::string::npos && header.find("/api") == std::string::npos)
			{
				continue;
			}

			bool bFound = false;
			std::string foundComp;
			for (auto comp : components)
			{
				if (header.find(comp) != std::string::npos)
				{
					bFound = true;
					foundComp = comp;
				}
			}

			if (bFound)
			{
				for (auto comp : components)
				{
					if (fileName.find(comp) != std::string::npos && comp != foundComp)
					{
						auto currentPair = std::make_pair(comp, foundComp);
						auto foundPair = componentsDependencies.find(currentPair);

						if (foundPair == componentsDependencies.cend())
						{
							std::set<std::string> set;
							set.insert(header);
							componentsDependencies.insert(std::make_pair(currentPair, set));
						}
						else
						{
							foundPair->second.insert(header);
						}

						break;
					}
				}
			}
		}
	}

	std::ofstream dep("componentsDepend.dot");
	dep << "digraph G \n{\n";
	for (auto it : componentsDependencies)
	{
		std::string strips1 = it.first.first.substr(it.first.first.find_last_of("\\") + 1);
		std::string strips2 = it.first.second.substr(it.first.second.find_last_of("\\") + 1);

		dep << "\"" << strips1 << "\"" << " -> " << "\"" << strips2 << "\"" << "[ label=\"";

		for (auto header : it.second)
		{
			std::string strips3 = header.substr(header.find_last_of("\\") + 1);

			dep << strips3 << ",";
		}
		dep << "\" ]" << std::endl;
	}
	dep << "}\n";
	
	printTree(tree, 0);
	//printLibrariesMap(processLibsMap, libsObjectsMap);
}