#pragma onceF

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Log
{
	enum class Type : uint8_t
	{
		Info,
		Warning,
		Error,
	};

	inline const std::string& msg(Type type)
	{
		static const std::unordered_map<Type, const std::string> msg = {
			{Type::Info, "<<INFO>> "},
			{Type::Warning, "<<WARNING>> "},
			{Type::Error, "<<ERROR>> "},
		};

		return msg.at(type);
	}

} // namespace Log
