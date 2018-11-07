
#include "habitat_interface.h"
#include "common/http_request.h"

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <sstream>
#include <chrono>
#include <thread>

#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "common/picosha2.h"


namespace
{

std::string sha256(const std::string& i_str)
{
    using namespace std;
    vector<unsigned char> hash(picosha2::k_digest_size);
    picosha2::hash256(i_str.cbegin(), i_str.cend(), hash.begin(), hash.end());
    return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
}


const char encodeLookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char padCharacter = '=';
std::string base64Encode(std::string inputBuffer)
{
    using namespace std;

	string encodedString;
	encodedString.reserve(((inputBuffer.size()/3) + (inputBuffer.size() % 3 > 0)) * 4);
	long temp;
	auto cursor = inputBuffer.begin();
	for(size_t idx = 0; idx < inputBuffer.size()/3; idx++)
	{
		temp  = (*cursor++) << 16; //Convert to big endian
		temp += (*cursor++) << 8;
		temp += (*cursor++);
		encodedString.append(1,encodeLookup[(temp & 0x00FC0000) >> 18]);
		encodedString.append(1,encodeLookup[(temp & 0x0003F000) >> 12]);
		encodedString.append(1,encodeLookup[(temp & 0x00000FC0) >> 6 ]);
		encodedString.append(1,encodeLookup[(temp & 0x0000003F)      ]);
	}
	switch(inputBuffer.size() % 3)
	{
	case 1:
		temp  = (*cursor++) << 16; //Convert to big endian
		encodedString.append(1,encodeLookup[(temp & 0x00FC0000) >> 18]);
		encodedString.append(1,encodeLookup[(temp & 0x0003F000) >> 12]);
		encodedString.append(2,padCharacter);
		break;
	case 2:
		temp  = (*cursor++) << 16; //Convert to big endian
		temp += (*cursor++) << 8;
		encodedString.append(1,encodeLookup[(temp & 0x00FC0000) >> 18]);
		encodedString.append(1,encodeLookup[(temp & 0x0003F000) >> 12]);
		encodedString.append(1,encodeLookup[(temp & 0x00000FC0) >> 6 ]);
		encodedString.append(1,padCharacter);
		break;
	}
	return encodedString;
}


std::string UtcNow()
{
    using namespace boost::posix_time;
    ptime now = second_clock::universal_time();
    return to_iso_extended_string(now) + "Z"; // habitat needs Z at the end
}


std::string SentenceToHabJson(
        const std::string& i_sentence,
        const std::string& i_listener_callsign
    )
{
    using namespace std;
    using namespace boost::posix_time;

    ptime now = second_clock::universal_time();
    string utc_now_str = to_iso_extended_string(now) + "Z"; // habitat needs Z at the end

    boost::property_tree::ptree root;
    root.put( "type", "payload_telemetry" );
    root.put( "data._raw", i_sentence );
    root.put( "receivers." + i_listener_callsign + ".time_created", utc_now_str );
    root.put( "receivers." + i_listener_callsign + ".time_uploaded", utc_now_str );

    stringstream ss;
    boost::property_tree::json_parser::write_json(ss, root);
    return ss.str();
}

} // namespace


namespace habdec
{

std::string HabitatUploadSentence(
        const std::string& i_sentence,
        const std::string& i_listener_callsign
    )
{
    using namespace std;

    string sentence = i_sentence;

    if( sentence.compare(0, 2, "$$") )
        sentence = "$$" + sentence;

    if( sentence.compare(sentence.size()-1, 1, "\n") )
        sentence += "\n";

    string sentence_b64 = base64Encode(sentence);
    string sentence_sha256 = sha256(sentence_b64);
    string sentence_json = SentenceToHabJson(sentence_b64, i_listener_callsign);

    string result;

    int retries_left = 5;
    while( retries_left-- )
    {
        int http_result = HttpRequest(
            "habitat.habhub.org",
            "/habitat/_design/payload_telemetry/_update/add_listener/" + sentence_sha256,
            80,
            habdec::HTTP_VERB::kPost,
            "application/json",
            sentence_json,
            result
        );

        switch( http_result )
        {
            case 201: // Success
            case 403: // Success, sentence has already seen by others
                retries_left = 0;
                break;
            case 409: // Upload conflict (server busy). Sleep for a moment, then retry
            default:
                this_thread::sleep_for( ( chrono::duration<double, milli>(300) ) );
                break;
        }
    }

    return result;
}

} // namespace habdec
