#pragma once

#if __cplusplus > 201703L
__inline QString from_u8(const char8_t* str)
{
	return QString::fromUtf8(reinterpret_cast<const char*>(str));
}
#else
__inline QString from_u8(const char* str)
{
	return QString::fromUtf8(str);
}
#endif


extern QDir openSubDir(const QDir& parent, const QString& dirName);

extern void xml_to_json(const QDomNode& node, QJsonObject& obj);