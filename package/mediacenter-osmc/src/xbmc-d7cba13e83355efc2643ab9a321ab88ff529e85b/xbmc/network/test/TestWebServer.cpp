/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <errno.h>
#include <stdlib.h>

#include <gtest/gtest.h>
#include "system.h"
#include "URL.h"
#include "filesystem/CurlFile.h"
#include "filesystem/File.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "network/WebServer.h"
#include "settings/MediaSourceSettings.h"
#include "test/TestUtils.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

using namespace XFILE;

#define WEBSERVER_HOST          "localhost"
#define WEBSERVER_PORT          23456

#define TEST_URL_JSONRPC        "jsonrpc"

#define TEST_FILES_DATA         "test"
#define TEST_FILES_DATA_RANGES  "range1;range2;range3"
#define TEST_FILES_HTML         TEST_FILES_DATA ".html"
#define TEST_FILES_RANGES       TEST_FILES_DATA "-ranges.txt"

class TestWebServer : public testing::Test
{
protected:
  TestWebServer()
    : webserver(),
      baseUrl(StringUtils::Format("http://" WEBSERVER_HOST ":%d", WEBSERVER_PORT)),
      sourcePath(XBMC_REF_FILE_PATH("xbmc/network/test/data/webserver/"))
  { }
  virtual ~TestWebServer() { }

protected:
  virtual void SetUp()
  {
    SetupMediaSources();

    webserver.Start(WEBSERVER_PORT, "", "");
  }

  virtual void TearDown()
  {
    if (webserver.IsStarted())
      webserver.Stop();

    TearDownMediaSources();
  }

  void SetupMediaSources()
  {
    CMediaSource source;
    source.strName = "WebServer Share";
    source.strPath = sourcePath;
    source.vecPaths.push_back(sourcePath);
    source.m_allowSharing = true;
    source.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    source.m_iLockMode = LOCK_MODE_EVERYONE;
    source.m_ignore = true;

    CMediaSourceSettings::GetInstance().AddShare("videos", source);
  }

  void TearDownMediaSources()
  {
    CMediaSourceSettings::GetInstance().Clear();
  }

  std::string GetUrl(const std::string& path)
  {
    if (path.empty())
      return baseUrl;

    return URIUtils::AddFileToFolder(baseUrl, path);
  }

  std::string GetUrlOfTestFile(const std::string& testFile)
  {
    if (testFile.empty())
      return "";

    std::string path = URIUtils::AddFileToFolder(sourcePath, testFile);
    path = CURL::Encode(path);
    path = URIUtils::AddFileToFolder("vfs", path);

    return GetUrl(path);
  }

  bool GetLastModifiedOfTestFile(const std::string& testFile, CDateTime& lastModified)
  {
    CFile file;
    if (!file.Open(URIUtils::AddFileToFolder(sourcePath, testFile), READ_NO_CACHE))
      return false;

    struct __stat64 statBuffer;
    if (file.Stat(&statBuffer) != 0)
      return false;

    struct tm *time;
#ifdef HAVE_LOCALTIME_R
    struct tm result = {};
    time = localtime_r((time_t*)&statBuffer.st_mtime, &result);
#else
    time = localtime((time_t *)&statBuffer.st_mtime);
#endif
    if (time == NULL)
      return false;

    lastModified = *time;
    return lastModified.IsValid();
  }

  void CheckHtmlTestFileResponse(const CCurlFile& curl)
  {
    // get the HTTP header details
    const CHttpHeader& httpHeader = curl.GetHttpHeader();

    // Content-Type must be "text/html"
    EXPECT_STREQ("text/html", httpHeader.GetMimeType().c_str());
    // Content-Length must be "4"
    EXPECT_STREQ("4", httpHeader.GetValue(MHD_HTTP_HEADER_CONTENT_LENGTH).c_str());
    // Accept-Ranges must be "bytes"
    EXPECT_STREQ("bytes", httpHeader.GetValue(MHD_HTTP_HEADER_ACCEPT_RANGES).c_str());

    // check Last-Modified
    CDateTime lastModified;
    ASSERT_TRUE(GetLastModifiedOfTestFile(TEST_FILES_HTML, lastModified));
    ASSERT_STREQ(lastModified.GetAsRFC1123DateTime().c_str(), httpHeader.GetValue(MHD_HTTP_HEADER_LAST_MODIFIED).c_str());

    // Cache-Control must contain "mag-age=0" and "no-cache"
    std::string cacheControl = httpHeader.GetValue(MHD_HTTP_HEADER_CACHE_CONTROL);
    EXPECT_TRUE(cacheControl.find("max-age=0") != std::string::npos);
    EXPECT_TRUE(cacheControl.find("no-cache") != std::string::npos);
  }

  void CheckRangesTestFileResponse(const CCurlFile& curl, int httpStatus = MHD_HTTP_OK, bool empty = false)
  {
    // get the HTTP header details
    const CHttpHeader& httpHeader = curl.GetHttpHeader();

    // check the protocol line for the expected HTTP status
    std::string httpStatusString = StringUtils::Format(" %d ", httpStatus);
    std::string protocolLine = httpHeader.GetProtoLine();
    ASSERT_TRUE(protocolLine.find(httpStatusString) != std::string::npos);

    // Content-Type must be "text/html"
    EXPECT_STREQ("text/plain", httpHeader.GetMimeType().c_str());
    // check Content-Length
    if (empty)
      EXPECT_STREQ("0", httpHeader.GetValue(MHD_HTTP_HEADER_CONTENT_LENGTH).c_str());
    else
      EXPECT_STREQ("20", httpHeader.GetValue(MHD_HTTP_HEADER_CONTENT_LENGTH).c_str());
    // Accept-Ranges must be "bytes"
    EXPECT_STREQ("bytes", httpHeader.GetValue(MHD_HTTP_HEADER_ACCEPT_RANGES).c_str());

    // check Last-Modified
    CDateTime lastModified;
    ASSERT_TRUE(GetLastModifiedOfTestFile(TEST_FILES_RANGES, lastModified));
    ASSERT_STREQ(lastModified.GetAsRFC1123DateTime().c_str(), httpHeader.GetValue(MHD_HTTP_HEADER_LAST_MODIFIED).c_str());

    // Cache-Control must contain "mag-age=0" and "no-cache"
    std::string cacheControl = httpHeader.GetValue(MHD_HTTP_HEADER_CACHE_CONTROL);
    EXPECT_TRUE(cacheControl.find("max-age=31536000") != std::string::npos);
    EXPECT_TRUE(cacheControl.find("public") != std::string::npos);
  }

  void CheckRangesTestFileResponse(const CCurlFile& curl, const std::string& result, const CHttpRanges& ranges)
  {
    // get the HTTP header details
    const CHttpHeader& httpHeader = curl.GetHttpHeader();

    // check the protocol line for the expected HTTP status
    std::string httpStatusString = StringUtils::Format(" %d ", MHD_HTTP_PARTIAL_CONTENT);
    std::string protocolLine = httpHeader.GetProtoLine();
    ASSERT_TRUE(protocolLine.find(httpStatusString) != std::string::npos);

    // Accept-Ranges must be "bytes"
    EXPECT_STREQ("bytes", httpHeader.GetValue(MHD_HTTP_HEADER_ACCEPT_RANGES).c_str());

    // check Last-Modified
    CDateTime lastModified;
    ASSERT_TRUE(GetLastModifiedOfTestFile(TEST_FILES_RANGES, lastModified));
    ASSERT_STREQ(lastModified.GetAsRFC1123DateTime().c_str(), httpHeader.GetValue(MHD_HTTP_HEADER_LAST_MODIFIED).c_str());

    // Cache-Control must contain "mag-age=0" and "no-cache"
    std::string cacheControl = httpHeader.GetValue(MHD_HTTP_HEADER_CACHE_CONTROL);
    EXPECT_TRUE(cacheControl.find("max-age=31536000") != std::string::npos);
    EXPECT_TRUE(cacheControl.find("public") != std::string::npos);

    // If there's no range Content-Length must be "20"
    if (ranges.IsEmpty())
    {
      EXPECT_STREQ("20", httpHeader.GetValue(MHD_HTTP_HEADER_CONTENT_LENGTH).c_str());
      EXPECT_STREQ(TEST_FILES_DATA_RANGES, result.c_str());
      return;
    }

    // check Content-Range
    uint64_t firstPosition, lastPosition;
    ASSERT_TRUE(ranges.GetFirstPosition(firstPosition));
    ASSERT_TRUE(ranges.GetLastPosition(lastPosition));
    EXPECT_STREQ(HttpRangeUtils::GenerateContentRangeHeaderValue(firstPosition, lastPosition, 20).c_str(), httpHeader.GetValue(MHD_HTTP_HEADER_CONTENT_RANGE).c_str());

    std::string expectedContent = TEST_FILES_DATA_RANGES;
    const std::string expectedContentType = "text/plain";
    if (ranges.Size() == 1)
    {
      // Content-Type must be "text/html"
      EXPECT_STREQ(expectedContentType.c_str(), httpHeader.GetMimeType().c_str());

      // check the content
      CHttpRange firstRange;
      ASSERT_TRUE(ranges.GetFirst(firstRange));
      expectedContent = expectedContent.substr(firstRange.GetFirstPosition(), firstRange.GetLength());
      EXPECT_STREQ(expectedContent.c_str(), result.c_str());

      // and Content-Length
      EXPECT_STREQ(StringUtils::Format("%u", static_cast<unsigned int>(expectedContent.size())).c_str(), httpHeader.GetValue(MHD_HTTP_HEADER_CONTENT_LENGTH).c_str());

      return;
    }

    // Content-Type contains the multipart boundary
    const std::string expectedMimeType = "multipart/byteranges";
    std::string mimeType = httpHeader.GetMimeType();
    ASSERT_STREQ(expectedMimeType.c_str(), mimeType.c_str());

    std::string contentType = httpHeader.GetValue(MHD_HTTP_HEADER_CONTENT_TYPE);
    std::string contentTypeStart = expectedMimeType + "; boundary=";
    // it must start with "multipart/byteranges; boundary=" followed by the boundary
    ASSERT_EQ(0, contentType.find(contentTypeStart));
    ASSERT_GT(contentType.size(), contentTypeStart.size());
    // extract the boundary
    std::string multipartBoundary = contentType.substr(contentTypeStart.size());
    ASSERT_FALSE(multipartBoundary.empty());
    multipartBoundary = "--" + multipartBoundary;

    ASSERT_EQ(0, result.find(multipartBoundary));
    std::vector<std::string> rangeParts = StringUtils::Split(result, multipartBoundary);
    // the first part is not really a part and is therefore empty (the place before the first boundary)
    ASSERT_TRUE(rangeParts.front().empty());
    rangeParts.erase(rangeParts.begin());
    // the last part is the end of the end multipart boundary
    ASSERT_STREQ("--", rangeParts.back().c_str());
    rangeParts.erase(rangeParts.begin() + rangeParts.size() - 1);
    ASSERT_EQ(ranges.Size(), rangeParts.size());

    for (size_t i = 0; i < rangeParts.size(); ++i)
    {
      std::string data = rangeParts.at(i);
      StringUtils::Trim(data, " \r\n");

      // find the separator between header and data
      size_t pos = data.find("\r\n\r\n");
      ASSERT_NE(std::string::npos, pos);

      std::string header = data.substr(0, pos + 4);
      data = data.substr(pos + 4);

      // get the expected range
      CHttpRange range;
      ASSERT_TRUE(ranges.Get(i, range));

      // parse the header of the range part
      CHttpHeader rangeHeader;
      rangeHeader.Parse(header);

      // check Content-Type
      EXPECT_STREQ(expectedContentType.c_str(), rangeHeader.GetMimeType().c_str());

      // parse and check Content-Range
      std::string contentRangeHeader = rangeHeader.GetValue(MHD_HTTP_HEADER_CONTENT_RANGE);
      std::vector<std::string> contentRangeHeaderParts = StringUtils::Split(contentRangeHeader, "/");
      ASSERT_EQ(2, contentRangeHeaderParts.size());

      // check the length of the range
      EXPECT_TRUE(StringUtils::IsNaturalNumber(contentRangeHeaderParts.back()));
      uint64_t contentRangeLength = str2uint64(contentRangeHeaderParts.back());
      EXPECT_EQ(range.GetLength(), contentRangeLength);

      // remove the leading "bytes " string from the range definition
      std::string contentRangeDefinition = contentRangeHeaderParts.front();
      ASSERT_EQ(0, contentRangeDefinition.find("bytes "));
      contentRangeDefinition = contentRangeDefinition.substr(6);

      // check the start and end positions of the range
      std::vector<std::string> contentRangeParts = StringUtils::Split(contentRangeDefinition, "-");
      ASSERT_EQ(2, contentRangeParts.size());
      EXPECT_TRUE(StringUtils::IsNaturalNumber(contentRangeParts.front()));
      uint64_t contentRangeStart = str2uint64(contentRangeParts.front());
      EXPECT_EQ(range.GetFirstPosition(), contentRangeStart);
      EXPECT_TRUE(StringUtils::IsNaturalNumber(contentRangeParts.back()));
      uint64_t contentRangeEnd = str2uint64(contentRangeParts.back());
      EXPECT_EQ(range.GetLastPosition(), contentRangeEnd);

      // make sure the length of the content matches the one of the expected range
      EXPECT_EQ(range.GetLength(), data.size());
      EXPECT_STREQ(expectedContent.substr(range.GetFirstPosition(), range.GetLength()).c_str(), data.c_str());
    }
  }

  std::string GenerateRangeHeaderValue(unsigned int start, unsigned int end)
  {
    return StringUtils::Format("bytes=%u-%u", start, end);
  }

  CWebServer webserver;
  std::string baseUrl;
  std::string sourcePath;
};

TEST_F(TestWebServer, IsStarted)
{
  ASSERT_TRUE(webserver.IsStarted());
}

TEST_F(TestWebServer, CanGetJsonRpcApiDescription)
{
  std::string result;
  CCurlFile curl;
  ASSERT_TRUE(curl.Get(GetUrl(TEST_URL_JSONRPC), result));
  ASSERT_FALSE(result.empty());

  // get the HTTP header details
  const CHttpHeader& httpHeader = curl.GetHttpHeader();

  // Content-Type must be "application/json"
  EXPECT_STREQ("application/json", httpHeader.GetMimeType().c_str());
  // Accept-Ranges must be "none"
  EXPECT_STREQ("none", httpHeader.GetValue(MHD_HTTP_HEADER_ACCEPT_RANGES).c_str());

  // Cache-Control must contain "mag-age=0" and "no-cache"
  std::string cacheControl = httpHeader.GetValue(MHD_HTTP_HEADER_CACHE_CONTROL);
  EXPECT_TRUE(cacheControl.find("max-age=0") != std::string::npos);
  EXPECT_TRUE(cacheControl.find("no-cache") != std::string::npos);
}

TEST_F(TestWebServer, CanGetJsonRpcResponse)
{
  // initialized JSON-RPC
  JSONRPC::CJSONRPC::Initialize();

  std::string result;
  CCurlFile curl;
  curl.SetMimeType("application/json");
  ASSERT_TRUE(curl.Post(GetUrl(TEST_URL_JSONRPC), "{ \"jsonrpc\": \"2.0\", \"method\": \"JSONRPC.Version\", \"id\": 1 }", result));
  ASSERT_FALSE(result.empty());

  // parse the JSON-RPC response
  CVariant resultObj = CJSONVariantParser::Parse(reinterpret_cast<const unsigned char*>(result.c_str()), result.size());
  // make sure it's an object
  ASSERT_TRUE(resultObj.isObject());

  // get the HTTP header details
  const CHttpHeader& httpHeader = curl.GetHttpHeader();

  // Content-Type must be "application/json"
  EXPECT_STREQ("application/json", httpHeader.GetMimeType().c_str());
  // Accept-Ranges must be "none"
  EXPECT_STREQ("none", httpHeader.GetValue(MHD_HTTP_HEADER_ACCEPT_RANGES).c_str());

  // Cache-Control must contain "mag-age=0" and "no-cache"
  std::string cacheControl = httpHeader.GetValue(MHD_HTTP_HEADER_CACHE_CONTROL);
  EXPECT_TRUE(cacheControl.find("max-age=0") != std::string::npos);
  EXPECT_TRUE(cacheControl.find("no-cache") != std::string::npos);

  // uninitialize JSON-RPC
  JSONRPC::CJSONRPC::Cleanup();
}

TEST_F(TestWebServer, CanNotHeadNonExistingFile)
{
  CCurlFile curl;
  ASSERT_FALSE(curl.Exists(CURL(GetUrlOfTestFile("file_does_not_exist"))));
}

TEST_F(TestWebServer, CanHeadFile)
{
  CCurlFile curl;
  ASSERT_TRUE(curl.Exists(CURL(GetUrlOfTestFile(TEST_FILES_HTML))));

  CheckHtmlTestFileResponse(curl);
}

TEST_F(TestWebServer, CanNotGetNonExistingFile)
{
  std::string result;
  CCurlFile curl;
  ASSERT_FALSE(curl.Get(GetUrlOfTestFile("file_does_not_exist"), result));
  ASSERT_TRUE(result.empty());
}

TEST_F(TestWebServer, CanGetFile)
{
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, "");
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_HTML), result));
  ASSERT_STREQ(TEST_FILES_DATA, result.c_str());

  CheckHtmlTestFileResponse(curl);
}

TEST_F(TestWebServer, CanGetFileForcingNoCache)
{
  // check non-cacheable HTML with Control-Cache: no-cache
  std::string result;
  CCurlFile curl_html;
  curl_html.SetRequestHeader(MHD_HTTP_HEADER_RANGE, "");
  curl_html.SetRequestHeader(MHD_HTTP_HEADER_CACHE_CONTROL, "no-cache");
  ASSERT_TRUE(curl_html.Get(GetUrlOfTestFile(TEST_FILES_HTML), result));
  EXPECT_STREQ(TEST_FILES_DATA, result.c_str());
  CheckHtmlTestFileResponse(curl_html);

  // check cacheable text file with Control-Cache: no-cache
  result.clear();
  CCurlFile curl_txt;
  curl_txt.SetRequestHeader(MHD_HTTP_HEADER_RANGE, "");
  curl_txt.SetRequestHeader(MHD_HTTP_HEADER_CACHE_CONTROL, "no-cache");
  ASSERT_TRUE(curl_txt.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  EXPECT_STREQ(TEST_FILES_DATA_RANGES, result.c_str());
  CheckRangesTestFileResponse(curl_txt);

  // check cacheable text file with deprecated Pragma: no-cache
  result.clear();
  CCurlFile curl_txt_pragma;
  curl_txt_pragma.SetRequestHeader(MHD_HTTP_HEADER_RANGE, "");
  curl_txt_pragma.SetRequestHeader(MHD_HTTP_HEADER_PRAGMA, "no-cache");
  ASSERT_TRUE(curl_txt_pragma.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  EXPECT_STREQ(TEST_FILES_DATA_RANGES, result.c_str());
  CheckRangesTestFileResponse(curl_txt_pragma);
}

TEST_F(TestWebServer, CanGetCachedFileWithOlderIfModifiedSince)
{
  // get the last modified date of the file
  CDateTime lastModified;
  ASSERT_TRUE(GetLastModifiedOfTestFile(TEST_FILES_RANGES, lastModified));
  CDateTime lastModifiedOlder = lastModified - CDateTimeSpan(1, 0, 0, 0);

  // get the file with an older If-Modified-Since value
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, "");
  curl.SetRequestHeader(MHD_HTTP_HEADER_IF_MODIFIED_SINCE, lastModifiedOlder.GetAsRFC1123DateTime());
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  EXPECT_STREQ(TEST_FILES_DATA_RANGES, result.c_str());
  CheckRangesTestFileResponse(curl);
}

TEST_F(TestWebServer, CanGetCachedFileWithExactIfModifiedSince)
{
  // get the last modified date of the file
  CDateTime lastModified;
  ASSERT_TRUE(GetLastModifiedOfTestFile(TEST_FILES_RANGES, lastModified));

  // get the file with the exact If-Modified-Since value
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, "");
  curl.SetRequestHeader(MHD_HTTP_HEADER_IF_MODIFIED_SINCE, lastModified.GetAsRFC1123DateTime());
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  ASSERT_TRUE(result.empty());
  CheckRangesTestFileResponse(curl, MHD_HTTP_NOT_MODIFIED, true);
}

TEST_F(TestWebServer, CanGetCachedFileWithNewerIfModifiedSince)
{
  // get the last modified date of the file
  CDateTime lastModified;
  ASSERT_TRUE(GetLastModifiedOfTestFile(TEST_FILES_RANGES, lastModified));
  CDateTime lastModifiedNewer = lastModified + CDateTimeSpan(1, 0, 0, 0);

  // get the file with a newer If-Modified-Since value
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, "");
  curl.SetRequestHeader(MHD_HTTP_HEADER_IF_MODIFIED_SINCE, lastModifiedNewer.GetAsRFC1123DateTime());
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  ASSERT_TRUE(result.empty());
  CheckRangesTestFileResponse(curl, MHD_HTTP_NOT_MODIFIED, true);
}

TEST_F(TestWebServer, CanGetCachedFileWithNewerIfModifiedSinceForcingNoCache)
{
  // get the last modified date of the file
  CDateTime lastModified;
  ASSERT_TRUE(GetLastModifiedOfTestFile(TEST_FILES_RANGES, lastModified));
  CDateTime lastModifiedNewer = lastModified + CDateTimeSpan(1, 0, 0, 0);

  // get the file with a newer If-Modified-Since value but forcing no caching
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, "");
  curl.SetRequestHeader(MHD_HTTP_HEADER_IF_MODIFIED_SINCE, lastModifiedNewer.GetAsRFC1123DateTime());
  curl.SetRequestHeader(MHD_HTTP_HEADER_CACHE_CONTROL, "no-cache");
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  EXPECT_STREQ(TEST_FILES_DATA_RANGES, result.c_str());
  CheckRangesTestFileResponse(curl);
}

TEST_F(TestWebServer, CanGetCachedFileWithOlderIfUnmodifiedSince)
{
  // get the last modified date of the file
  CDateTime lastModified;
  ASSERT_TRUE(GetLastModifiedOfTestFile(TEST_FILES_RANGES, lastModified));
  CDateTime lastModifiedOlder = lastModified - CDateTimeSpan(1, 0, 0, 0);

  // get the file with an older If-Unmodified-Since value
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, "");
  curl.SetRequestHeader(MHD_HTTP_HEADER_IF_UNMODIFIED_SINCE, lastModifiedOlder.GetAsRFC1123DateTime());
  ASSERT_FALSE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
}

TEST_F(TestWebServer, CanGetCachedFileWithExactIfUnmodifiedSince)
{
  // get the last modified date of the file
  CDateTime lastModified;
  ASSERT_TRUE(GetLastModifiedOfTestFile(TEST_FILES_RANGES, lastModified));

  // get the file with an older If-Unmodified-Since value
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, "");
  curl.SetRequestHeader(MHD_HTTP_HEADER_IF_UNMODIFIED_SINCE, lastModified.GetAsRFC1123DateTime());
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  EXPECT_STREQ(TEST_FILES_DATA_RANGES, result.c_str());
  CheckRangesTestFileResponse(curl);
}

TEST_F(TestWebServer, CanGetCachedFileWithNewerIfUnmodifiedSince)
{
  // get the last modified date of the file
  CDateTime lastModified;
  ASSERT_TRUE(GetLastModifiedOfTestFile(TEST_FILES_RANGES, lastModified));
  CDateTime lastModifiedNewer = lastModified + CDateTimeSpan(1, 0, 0, 0);

  // get the file with a newer If-Unmodified-Since value
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, "");
  curl.SetRequestHeader(MHD_HTTP_HEADER_IF_UNMODIFIED_SINCE, lastModifiedNewer.GetAsRFC1123DateTime());
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  EXPECT_STREQ(TEST_FILES_DATA_RANGES, result.c_str());
  CheckRangesTestFileResponse(curl);
}

TEST_F(TestWebServer, CanGetRangedFileRange0_)
{
  const std::string rangedFileContent = TEST_FILES_DATA_RANGES;
  const std::string range = "bytes=0-";

  CHttpRanges ranges;
  ASSERT_TRUE(ranges.Parse(range, rangedFileContent.size()));

  // get the whole file but specify the beginning of the range
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, range);
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  CheckRangesTestFileResponse(curl, result, ranges);
}

TEST_F(TestWebServer, CanGetRangedFileRange0_End)
{
  const std::string rangedFileContent = TEST_FILES_DATA_RANGES;
  const std::string range = GenerateRangeHeaderValue(0, rangedFileContent.size());

  CHttpRanges ranges;
  ASSERT_TRUE(ranges.Parse(range, rangedFileContent.size()));

  // get the whole file but specify the whole range
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, range);
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  CheckRangesTestFileResponse(curl, result, ranges);
}

TEST_F(TestWebServer, CanGetRangedFileRange0_2xEnd)
{
  const std::string rangedFileContent = TEST_FILES_DATA_RANGES;
  const std::string range = GenerateRangeHeaderValue(0, rangedFileContent.size() * 2);

  CHttpRanges ranges;
  ASSERT_TRUE(ranges.Parse(range, rangedFileContent.size()));

  // get the whole file but specify a larger range
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, range);
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  CheckRangesTestFileResponse(curl, result, ranges);
}

TEST_F(TestWebServer, CanGetRangedFileRange0_First)
{
  const std::string rangedFileContent = TEST_FILES_DATA_RANGES;
  std::vector<std::string> rangedContent = StringUtils::Split(TEST_FILES_DATA_RANGES, ";");
  const std::string range = GenerateRangeHeaderValue(0, rangedContent.front().size() - 1);

  CHttpRanges ranges;
  ASSERT_TRUE(ranges.Parse(range, rangedFileContent.size()));

  // get the whole file but specify a larger range
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, range);
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  CheckRangesTestFileResponse(curl, result, ranges);
}

TEST_F(TestWebServer, CanGetRangedFileRangeFirst_Second)
{
  const std::string rangedFileContent = TEST_FILES_DATA_RANGES;
  std::vector<std::string> rangedContent = StringUtils::Split(TEST_FILES_DATA_RANGES, ";");
  const std::string range = GenerateRangeHeaderValue(rangedContent.front().size() + 1, rangedContent.front().size() + 1 + rangedContent.at(2).size() - 1);

  CHttpRanges ranges;
  ASSERT_TRUE(ranges.Parse(range, rangedFileContent.size()));

  // get the whole file but specify a larger range
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, range);
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  CheckRangesTestFileResponse(curl, result, ranges);
}

TEST_F(TestWebServer, CanGetRangedFileRange_Last)
{
  const std::string rangedFileContent = TEST_FILES_DATA_RANGES;
  std::vector<std::string> rangedContent = StringUtils::Split(TEST_FILES_DATA_RANGES, ";");
  const std::string range = StringUtils::Format("bytes=-%u", static_cast<unsigned int>(rangedContent.back().size()));

  CHttpRanges ranges;
  ASSERT_TRUE(ranges.Parse(range, rangedFileContent.size()));

  // get the whole file but specify a larger range
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, range);
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  CheckRangesTestFileResponse(curl, result, ranges);
}

TEST_F(TestWebServer, CanGetRangedFileRangeFirstSecond)
{
  const std::string rangedFileContent = TEST_FILES_DATA_RANGES;
  std::vector<std::string> rangedContent = StringUtils::Split(TEST_FILES_DATA_RANGES, ";");
  const std::string range = StringUtils::Format("bytes=0-%u,%u-%u", static_cast<unsigned int>(rangedContent.front().size() - 1),
    static_cast<unsigned int>(rangedContent.front().size() + 1), static_cast<unsigned int>(rangedContent.front().size() + 1) + static_cast<unsigned int>(rangedContent.at(1).size() - 1));

  CHttpRanges ranges;
  ASSERT_TRUE(ranges.Parse(range, rangedFileContent.size()));

  // get the whole file but specify a larger range
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, range);
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  CheckRangesTestFileResponse(curl, result, ranges);
}

TEST_F(TestWebServer, CanGetRangedFileRangeFirstSecondLast)
{
  const std::string rangedFileContent = TEST_FILES_DATA_RANGES;
  std::vector<std::string> rangedContent = StringUtils::Split(TEST_FILES_DATA_RANGES, ";");
  const std::string range = StringUtils::Format("bytes=0-%u,%u-%u,-%u", static_cast<unsigned int>(rangedContent.front().size() - 1),
    static_cast<unsigned int>(rangedContent.front().size() + 1), static_cast<unsigned int>(rangedContent.front().size() + 1) + static_cast<unsigned int>(rangedContent.at(1).size() - 1),
    static_cast<unsigned int>(rangedContent.back().size()));

  CHttpRanges ranges;
  ASSERT_TRUE(ranges.Parse(range, rangedFileContent.size()));

  // get the whole file but specify a larger range
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, range);
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  CheckRangesTestFileResponse(curl, result, ranges);
}

TEST_F(TestWebServer, CanGetCachedRangedFileWithOlderIfRange)
{
  const std::string rangedFileContent = TEST_FILES_DATA_RANGES;
  const std::string range = "bytes=0-";

  CHttpRanges ranges;
  ASSERT_TRUE(ranges.Parse(range, rangedFileContent.size()));

  // get the last modified date of the file
  CDateTime lastModified;
  ASSERT_TRUE(GetLastModifiedOfTestFile(TEST_FILES_RANGES, lastModified));
  CDateTime lastModifiedOlder = lastModified - CDateTimeSpan(1, 0, 0, 0);

  // get the whole file (but ranged) with an older If-Range value
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, range);
  curl.SetRequestHeader(MHD_HTTP_HEADER_IF_RANGE, lastModifiedOlder.GetAsRFC1123DateTime());
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  EXPECT_STREQ(TEST_FILES_DATA_RANGES, result.c_str());
  CheckRangesTestFileResponse(curl);
}

TEST_F(TestWebServer, CanGetCachedRangedFileWithExactIfRange)
{
  const std::string rangedFileContent = TEST_FILES_DATA_RANGES;
  const std::string range = "bytes=0-";

  CHttpRanges ranges;
  ASSERT_TRUE(ranges.Parse(range, rangedFileContent.size()));

  // get the last modified date of the file
  CDateTime lastModified;
  ASSERT_TRUE(GetLastModifiedOfTestFile(TEST_FILES_RANGES, lastModified));

  // get the whole file (but ranged) with an older If-Range value
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, range);
  curl.SetRequestHeader(MHD_HTTP_HEADER_IF_RANGE, lastModified.GetAsRFC1123DateTime());
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  CheckRangesTestFileResponse(curl, result, ranges);
}

TEST_F(TestWebServer, CanGetCachedRangedFileWithNewerIfRange)
{
  const std::string rangedFileContent = TEST_FILES_DATA_RANGES;
  const std::string range = "bytes=0-";

  CHttpRanges ranges;
  ASSERT_TRUE(ranges.Parse(range, rangedFileContent.size()));

  // get the last modified date of the file
  CDateTime lastModified;
  ASSERT_TRUE(GetLastModifiedOfTestFile(TEST_FILES_RANGES, lastModified));
  CDateTime lastModifiedNewer = lastModified + CDateTimeSpan(1, 0, 0, 0);

  // get the whole file (but ranged) with an older If-Range value
  std::string result;
  CCurlFile curl;
  curl.SetRequestHeader(MHD_HTTP_HEADER_RANGE, range);
  curl.SetRequestHeader(MHD_HTTP_HEADER_IF_RANGE, lastModifiedNewer.GetAsRFC1123DateTime());
  ASSERT_TRUE(curl.Get(GetUrlOfTestFile(TEST_FILES_RANGES), result));
  CheckRangesTestFileResponse(curl, result, ranges);
}