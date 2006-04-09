#ifndef PDNS_DNSWRITER_HH
#define PDNS_DNSWRITER_HH

#include <string>
#include <vector>
#include <map>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/nameser.h>

using namespace std;

/** this class can be used to write DNS packets. It knows about DNS in the sense that it makes 
    the packet header and record headers.

    The model is:

    packetheader (recordheader recordcontent)*

    The packetheader needs to be updated with the amount of packets of each kind (answer, auth, additional)
    
    Each recordheader contains the length of a dns record.

    Calling convention:

    vector<uint8_t> content;
    DNSPacketWriter dpw(content, const string& qname, uint16_t qtype, uint16_t qclass=1);  // sets the question
    dpw.startrecord("this.is.an.ip.address.", ns_t_a);    // does nothing, except store qname and qtype
    dpw.xfr32BitInt(0x01020304);                         // adds 4 bytes (0x01020304) to the record buffer
    dpw.startrecord("this.is.an.ip.address.", ns_t_a);    // aha! writes out dnsrecord header containing qname and qtype and length 4, plus the recordbuffer, which gets emptied
                                                         // new qname and qtype are stored
    dpw.xfr32BitInt(0x04030201);                         // adds 4 bytes (0x04030201) to the record buffer
    dpw.commit();                                        // writes out dnsrecord header containing qname and qtype and length 4, plus the recordbuffer

    // content now contains the ready packet, with 1 question and 2 answers

*/

class DNSPacketWriter
{
public:
  typedef HEADER dnsheader;

  enum Place {ANSWER=1, AUTHORITY=2, ADDITIONAL=3}; 

  //! Start a DNS Packet in the vector passed, with question qname, qtype and qclass
  DNSPacketWriter(vector<uint8_t>& content, const string& qname, uint16_t  qtype, uint16_t qclass=1);
  
  /** Start a new DNS record within this packet for namq, qtype, ttl, class and in the requested place. Note that packets can only be written in natural order - 
      ANSWER, AUTHORITY, ADDITIONAL */
  void startRecord(const string& name, uint16_t qtype, uint32_t ttl=3600, uint16_t qclass=1, Place place=ANSWER);

  /** Shorthand way to add an Opt-record, for example for EDNS0 purposes */
  void addOpt(int udpsize, int extRCode, int Z);

  /** needs to be called after the last record is added, but can be called again and again later on. Is called internally by startRecord too.
      The content of the vector<> passed to the constructor is inconsistent until commit is called.
   */
  void commit();

  uint16_t size();

  /** Should the packet have grown too big for the writer's liking, rollback removes the record currently being written */
  void rollback();

  void xfr32BitInt(uint32_t val);
  void xfr16BitInt(uint16_t val);
  void xfrType(uint16_t val)
  {
    xfr16BitInt(val);
  }
  void xfrIP(const uint32_t& val)
  {
    xfr32BitInt(htonl(val));
  }
  void xfrTime(const uint32_t& val)
  {
    xfr32BitInt(val);
  }

  void xfr8BitInt(uint8_t val);

  void xfrLabel(const string& label, bool compress=false);
  void xfrText(const string& text);
  void xfrBlob(const string& blob);
  void xfrHexBlob(const string& blob);

  uint16_t d_pos;
  
  dnsheader* getHeader();
  void getRecords(string& records);

private:
  vector<uint8_t>& d_content;
  vector <uint8_t> d_record;
  string d_qname;
  uint16_t d_qtype, d_qclass;
  string d_recordqname;
  uint16_t d_recordqtype, d_recordqclass;
  uint32_t d_recordttl;
  map<string, uint16_t> d_labelmap;
  uint16_t d_stuff;
  uint16_t d_sor;
  uint16_t d_rollbackmarker; // start of last complete packet, for rollback
  Place d_recordplace;
};
#endif
