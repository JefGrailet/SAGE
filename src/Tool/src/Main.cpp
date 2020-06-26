#include <cstdlib>
#include <set>
using std::set;
#include <ctime>
#include <iostream>
using std::cout;
using std::cin;
using std::endl;
using std::exit;
#include <fstream>
using std::ofstream;
using std::ios;
#include <string>
using std::string;
#include <sstream>
using std::stringstream;
#include <list>
using std::list;
#include <algorithm> // For transform() function
#include <getopt.h> // For options parsing
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <ctime> // To obtain current time (for output file)

#include "common/inet/InetAddress.h"
#include "common/inet/InetAddressException.h"
#include "common/inet/NetworkAddress.h"
#include "common/thread/Thread.h"
#include "common/thread/Runnable.h"
#include "common/thread/Mutex.h"
#include "common/thread/MutexException.h"
#include "common/date/TimeVal.h"
#include "common/utils/StringUtils.h"
#include "prober/icmp/DirectICMPProber.h"

#include "algo/Environment.h"
#include "algo/utils/ConfigFileParser.h"
#include "algo/utils/TargetParser.h"
#include "algo/prescanning/TargetPrescanner.h"
#include "algo/scanning/TargetScanner.h"
#include "algo/subnetinference/SubnetInferrer.h"
#include "algo/subnetinference/SubnetPostProcessor.h"
#include "algo/graph/PeerScanner.h"
#include "algo/graph/TopologyInferrer.h"

#include "algo/graph/voyagers/Mariner.h"
#include "algo/graph/voyagers/Cassini.h"
#include "algo/graph/voyagers/Galileo.h"

// Simple functions to display program summary and usage.

void printInfo()
{
    cout << "Summary\n";
    cout << "=======\n";
    cout << "\n";
    cout << "SAGE (Subnet AGgrEgation) is a topology discovery tool built on top of WISE\n";
    cout << "(Wide and lInear Subnet inferencE; a subnet inference tool) which is able to\n";
    cout << "discover subnets within a target domain in a time proportional to the amount\n";
    cout << "of IPs that are responsive to probes in the same network then infer a graph\n";
    cout << "that approximates its topology, using lightweight additional probing. SAGE\n";
    cout << "consists in practice in a succession of algorithmic steps that are briefly\n";
    cout << "described below.\n";
    cout << "\n";
    cout << "Algorithmic steps\n";
    cout << "=================\n";
    cout << "\n";
    cout << "0) Launch and target selection\n";
    cout << "------------------------------\n";
    cout << "\n";
    cout << "SAGE parses its main argument to get a list of all the target IPs it should\n";
    cout << "consider in subsequent steps.\n";
    cout << "\n";
    cout << "1) Target pre-scanning\n";
    cout << "----------------------\n";
    cout << "\n";
    cout << "Each target IP is probed once to evaluate its liveness. Unresponsive IPs are\n";
    cout << "probed a second time with twice the initial timeout as a second opinion (note:\n";
    cout << "a third opinion can be requested via a configuration file). Only IPs that were\n";
    cout << "responsive during this step will be probed again during the next steps,\n";
    cout << "avoiding useless probing work on unresponsive IPs. Several target IPs are\n";
    cout << "probed at the same time, via multi-threading.\n";
    cout << "\n";
    cout << "2) Target scanning\n";
    cout << "------------------\n";
    cout << "\n";
    cout << "Responsive target IPs are probed furthermore in order to get two pieces of\n";
    cout << "information: the distance from the vantage point (as a TTL value below which\n";
    cout << "probing the IP will result in a \"Time exceeded\" reply) and the trail. The\n";
    cout << "trail is a portion of the route towards a target IP which consists of the last\n";
    cout << "non-anonymous and non-cycling interface found in the route associated to an\n";
    cout << "amount of anomalies, anomalies being cycles or anonymous hops. Ideally, a\n";
    cout << "trail is just the last hop before the destination IP, the concept of anomalies\n";
    cout << "being a way to denote trails for which the last hop of the route can't be\n";
    cout << "obtained (this can happen because router configuration or traffic issues) and\n";
    cout << "use them in subsequent algorithmic step(s) in a \"best effort\" fashion.\n";
    cout << "\n";
    cout << "Target scanning is carried out with multi-threading, each thread probing a set\n";
    cout << "of consecutive IPs (w.r.t. the IPv4 address space) and using their respective\n";
    cout << "distance as a way to reduce the probing work. Indeed, in the hypothesis of\n";
    cout << "such IPs belonging to the same subnet, they should be located at similar TTL\n";
    cout << "distances, and therefore, only one of these IPs should have its distance\n";
    cout << "estimation fully carried (i.e. the starting TTL for probes will be the minimal\n";
    cout << "one). The distance towards subsequent IPs can be done by using the previous\n";
    cout << "TTL distance as a basis (rather than starting TTL), therefore reducing the\n";
    cout << "total amount of probes.\n";
    cout << "\n";
    cout << "At the end of target scanning, the updated IP dictionary is processed in order\n";
    cout << "to detect special IPs, such as IPs which appear at several different distances\n";
    cout << "(called \"warping\" IPs), IPs which appear as the last hop towards themselves\n";
    cout << "(\"echoing\" IPs) or IPs appearing in trails (with no anomalies) for IPs that\n";
    cout << "are close and consecutive in the address space and alternating between each\n";
    cout << "other (also named \"flickering\" IPs).\n";
    cout << "\n";
    cout << "3) Subnet inference\n";
    cout << "-------------------\n";
    cout << "\n";
    cout << "The data collected at the previous step is used as a basis to perform an\n";
    cout << "offline subnet inference, with the exception of a preliminary step where\n";
    cout << "flickering IPs are probed to conduct alias resolution on them (as such IPs\n";
    cout << "could very well belong to the same device due to their locations in the\n";
    cout << "network).\n";
    cout << "\n";
    cout << "Then, the IP dictionary is processed in a linear manner to build subnets by\n";
    cout << "aggregating IPs which are consecutive w.r.t. the address space and which the\n";
    cout << "data respect a set of established subnet rules (e.g., same trail, same TTL\n";
    cout << "with flickering trail but trail IPs were aliased, etc.). A subnet is expanded\n";
    cout << "as long as most of the newly added IPs are a majority to fulfill subnet rules\n";
    cout << "and as long as no contra-pivot IP (i.e. an IP that belongs to the router\n";
    cout << "providing access to the subnet, most of the time seen one hop sooner TTL-wise)\n";
    cout << "has been discovered.\n";
    cout << "\n";
    cout << "The inference finishes with a post-processing stage which processes subnets\n";
    cout << "in a sense opposite of the one used to process IPs previously, in order to\n";
    cout << "aggregate subnets that are undergrown into larger ones (this can happen due\n";
    cout << "to well located outliers) but also to re-size subnets to the minimal prefix\n";
    cout << "length that accomodate all their respective interfaces.\n";
    cout << "\n";
    cout << "4) Neighborhood inference\n";
    cout << "-------------------------\n";
    cout << "\n";
    cout << "Using the data collected so far (including the subnets themselves) and some\n";
    cout << "additional probing, SAGE aggregates subnets together into \"neighborhoods\",\n";
    cout << "i.e., network locations where subnets are located at at most one hop from\n";
    cout << "each other, which should correspond to a single router or a mesh of L2/L3\n";
    cout << "devices.\n";
    cout << "\n";
    cout << "SAGE discovers all neighborhoods as well as hints of how they are located with\n";
    cout << "respect to each others, using the concept of \"peer\": if a route hop leading\n";
    cout << "towards pivot(s) (seen just before their trails) of some subnets aggregated\n";
    cout << "into a neighborhood is also used to identify another neighborhood, then both\n";
    cout << "neighborhoods are located next to each other. Such knowledge is used to build\n";
    cout << "a neighborhood-based graph of the measured target domain, where vertices model\n";
    cout << "the neighborhoods while the edges model the links which connect them together.\n";
    cout << "The construction itself works in several algorithmic steps, with one of them\n";
    cout << "relying on alias resolution to ensure initially discovered neighborhoods are\n";
    cout << "not themselves parts of larger neighborhoods (this can notably happen when\n";
    cout << "evaluating networks which implement traffic engineering policies).\n";
    cout << "\n";
    cout << "5) Final alias resolution\n";
    cout << "-------------------------\n";
    cout << "\n";
    cout << "Once the network graph is fully built, SAGE processes it in order to perform\n";
    cout << "alias resolution within each neighborhood. Indeed, both the contra-pivot IPs\n";
    cout << "of the subnets and the IP(s) used to identify the neighborhood are potential\n";
    cout << "aliases of each others. The discovery of neighborhoods is, in fact, a form of\n";
    cout << "space search reduction for alias resolution performed in the wild, as trying\n";
    cout << "to alias IPs in the wild regardless of their location can be prohibitive in\n";
    cout << "terms of probing for specific techniques, such as Ally. Combining the network\n";
    cout << "graph (neighborhood-based) and the aliases discovered during this fifth step\n";
    cout << "offers multiple modeling possibilities. Note that this approach for alias\n";
    cout << "resolution (i.e., combining space search reduction and fingerprinting) has\n";
    cout << "been already used by another topology discovery tool (TreeNET).\n";
    cout.flush();
}

void printUsage()
{
    cout << "Usage\n";
    cout << "=====\n";
    cout << "\n";
    cout << "You can use this tool as follows:\n";
    cout << "\n";
    cout << "./sage [target n°1],[target n°2],[...]\n";
    cout << "\n";
    cout << "where each target can be:\n";
    cout << "-a single IP,\n";
    cout << "-a whole IP block (in CIDR notation),\n";
    cout << "-a file containing a list of the notations mentioned above, which each item\n";
    cout << " being separated with \\n.\n";
    cout << "\n";
    cout << "You can use various options and flags to handle the main settings, such as the\n";
    cout << "probing protocol, the label of the output files, etc. These options and flags\n";
    cout << "are detailed below. Note, however, that probing or algorithmic parameters can\n";
    cout << "only be handled by providing a separate configuration file. Default parameters\n";
    cout << "will be used if no configuration file is provided.\n";
    cout << "\n";
    cout << "Short   Verbose                             Expected value\n";
    cout << "-----   -------                             --------------\n";
    cout << "\n";
    cout << "-c      --configuration-file                Path to a configuration file\n";
    cout << "\n";
    cout << "Use this option to feed a configuration file to SAGE. Use such a file,\n";
    cout << "formatted as key=value pairs (one per line), to edit probing, concurrency or\n";
    cout << "algorithmic parameters. Refer to the documentation of SAGE to learn about the\n";
    cout << "keys you can use and the values you can provide for them.\n";
    cout << "\n";
    cout << "-e      --probing-egress-interface          IP or DNS\n";
    cout << "\n";
    cout << "Interface name through which probing/response packets exit/enter (default is\n";
    cout << "the first non-loopback IPv4 interface in the active interface list). Use this\n";
    cout << "option if your machine has multiple network interface cards and if you want to\n";
    cout << "prefer one interface over the others.\n";
    cout << "\n";
    cout << "-p      --probing-protocol                  \"ICMP\", \"UDP\" or \"TCP\"\n";
    cout << "\n";
    cout << "Use this option to specify the base protocol used to probe target addresses.\n";
    cout << "By default, it is ICMP. Note that some inference techniques (e.g. during alias\n";
    cout << "resolution) rely on a precise protocol which is therefore used instead.\n";
    cout << "\n";
    cout << "WARNING for TCP probing: keep in mind that the TCP probing consists in sending\n";
    cout << "a SYN message to the target, without handling the 3-way handshake properly in\n";
    cout << "case of a SYN+ACK reply. Repeated probes towards a same IP (which can occur\n";
    cout << "during alias resolution) can also be identified as SYN flooding, which is a\n";
    cout << "type of denial-of-service attack. Please consider security issues carefully\n";
    cout << "before using this probing method.\n";
    cout << "\n";
    cout << "-l      --label-output                      String\n";
    cout << "\n";
    cout << "Use this option to edit the label that will be used to name the various output\n";
    cout << "files. By default, it will use the time at which SAGE first started to run,\n"; 
    cout << "in the format dd-mm-yyyy hh:mm:ss.\n";
    cout << "\n";
    cout << "-v      --verbosity                         0, 1, 2 or 3\n";
    cout << "\n";
    cout << "Use this option to handle the verbosity of the console output. 3 amounts to\n";
    cout << "debug mode.\n";
    cout << "\n";
    cout << "-h      --help                              None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to know how to use this tool and see the\n";
    cout << "complete list of options and flags and how they work. It will not run further\n";
    cout << "after displaying this, though the -i flag can be used in addition.\n";
    cout << "\n";
    cout << "-i      --info                              None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to read a summary of how this tool works.\n";
    cout << "\n";
    cout << "About\n";
    cout << "=====\n";
    cout << "\n";
    cout << "SAGE (Subnet AGgrEgation) is a topology discovery tool built on top of WISE\n";
    cout << "(Wide and lInear Subnet inferencE), a subnet inference tool. Both tools were\n";
    cout << "designed and implemented by Jean-François Grailet (Ph. D. student at the\n";
    cout << "University of Liège).\n";
    cout << "\n";
    
    cout.flush();
}

// Simple function to get the current time as a string object.

string getCurrentTimeStr()
{
    time_t rawTime;
    struct tm *timeInfo;
    char buffer[80];

    time(&rawTime);
    timeInfo = localtime(&rawTime);

    strftime(buffer, 80, "%d-%m-%Y %T", timeInfo);
    string timeStr(buffer);
    
    return timeStr;
}

// Simple function to convert an elapsed time (in seconds) into days/hours/mins/secs format

string elapsedTimeStr(unsigned long elapsedSeconds)
{
    if(elapsedSeconds == 0)
    {
        return "less than one second";
    }

    unsigned short secs = (unsigned short) elapsedSeconds % 60;
    unsigned short mins = (unsigned short) (elapsedSeconds / 60) % 60;
    unsigned short hours = (unsigned short) (elapsedSeconds / 3600) % 24;
    unsigned short days = (unsigned short) elapsedSeconds / 86400;
    
    stringstream ss;
    if(days > 0)
    {
        if(days > 1)
            ss << days << " days ";
        else
            ss << "1 day ";
    }
    if(hours > 0)
    {
        if(hours > 1)
            ss << hours << " hours ";
        else
            ss << "1 hour ";
    }
    if(mins > 0)
    {
        if(mins > 1)
            ss << mins << " minutes ";
        else
            ss << "1 minute ";
    }
    if(secs > 1)
        ss << secs << " seconds";
    else
        ss << secs << " second";
    
    return ss.str();    
}

// Main function; deals with inputs and launches thread(s)

int main(int argc, char *argv[])
{
    // Default parameters for options which can be edited by user in the command line
    string configFilePath = "";
    InetAddress localIPAddress;
    unsigned char LANSubnetMask = 0;
    unsigned short probingProtocol = Environment::PROBING_PROTOCOL_ICMP;
    unsigned short displayMode = Environment::DISPLAY_MODE_LACONIC;
    string outputFileName = ""; // Gets a default value later if not set by user.
    
    // Values to check if algorithmic summary or usage should be displayed.
    bool displayInfo = false, displayUsage = false;
    
    /*
     * PARSING ARGUMENT
     * 
     * The main argument (target prefixes or input files, each time separated by commas) can be 
     * located anywhere. To make things simple for getopt_long(), argv is processed to find it and 
     * put it at the end. If not found, the program stops and displays an error message.
     */
    
    int totalArgs = argc;
    string targetsStr = ""; // List of targets (input files or plain targets)
    bool found = false;
    bool flagParam = false; // True if a value for a parameter is expected
    for(int i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'h':
                case 'i':
                    break;
                default:
                    flagParam = true;
                    break;
            }
        }
        else if(flagParam)
        {
            flagParam = false;
        }
        else
        {
            // Argument found!
            char *arg = argv[i];
            for(int j = i; j < argc - 1; j++)
                argv[j] = argv[j + 1];
            argv[argc - 1] = arg;
            found = true;
            totalArgs--;
            break;
        }
    }
    
    targetsStr = argv[argc - 1];
    
    /*
     * PARSING PARAMETERS
     *
     * In addition to the main argument parsed above, this program provides various input flags 
     * which can be used to handle some essential parameters. Probing, algorithmic or concurrency 
     * parameters must be edited via a configuration file (see ConfigFileParser, Environment 
     * classes).
     */
     
    int opt = 0;
    int longIndex = 0;
    const char* const shortOpts = "c:e:hil:p:v:";
    const struct option longOpts[] = {
            {"configuration-file", required_argument, NULL, 'c'}, 
            {"probing-egress-interface", required_argument, NULL, 'e'}, 
            {"probing-protocol", required_argument, NULL, 'p'}, 
            {"label-output", required_argument, NULL, 'l'}, 
            {"verbosity", required_argument, NULL, 'v'}, 
            {"help", no_argument, NULL, 'h'}, 
            {"info", no_argument, NULL, 'i'},
            {NULL, 0, NULL, 0}
    };
    
    string optargSTR;
    try
    {
        while((opt = getopt_long(totalArgs, argv, shortOpts, longOpts, &longIndex)) != -1)
        {
            /*
             * Beware: use the line optargSTR = string(optarg); ONLY for flags WITH arguments !! 
             * Otherwise, it prevents the code from recognizing flags like -v, -h or -g (because 
             * they require no argument) and make it throw an exception... To avoid this, a second 
             * switch is used.
             *
             * (this error is still present in ExploreNET v2.1)
             */
            
            switch(opt)
            {
                case 'h':
                case 'i':
                case 'j':
                case 'k':
                    break;
                default:
                    optargSTR = string(optarg);
                    
                    /*
                     * For future readers: optarg is of type extern char*, and is defined in getopt.h.
                     * Therefore, you will not find the declaration of this variable in this file.
                     */
                    
                    break;
            }
            
            // Now we can actually treat the options.
            int gotNb = 0;
            switch(opt)
            {
                case 'c':
                    configFilePath = optargSTR;
                    break;
                case 'e':
                    try
                    {
                        localIPAddress = InetAddress::getLocalAddressByInterfaceName(optargSTR);
                    }
                    catch (const InetAddressException &e)
                    {
                        cout << "Error for -e option: cannot obtain any IP address ";
                        cout << "assigned to the interface \"" + optargSTR + "\". ";
                        cout << "Please fix the argument for this option before ";
                        cout << "restarting.\n" << endl;
                        return 1;
                    }
                    break;
                case 'p':
                    std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                    if(optargSTR == string("UDP"))
                    {
                        probingProtocol = Environment::PROBING_PROTOCOL_UDP;
                    }
                    else if(optargSTR == string("TCP"))
                    {
                        probingProtocol = Environment::PROBING_PROTOCOL_TCP;
                    }
                    else if(optargSTR != string("ICMP"))
                    {
                        cout << "Warning for option -b: unrecognized protocol " << optargSTR;
                        cout << ". Please select a protocol between the following three: ";
                        cout << "ICMP, UDP and TCP. Note that ICMP is the default base ";
                        cout << "protocol.\n" << endl;
                    }
                    break;
                case 'l':
                    outputFileName = optargSTR;
                    break;
                case 'v':
                    gotNb = std::atoi(optargSTR.c_str());
                    if(gotNb >= 0 && gotNb <= 3)
                        displayMode = (unsigned short) gotNb;
                    else
                    {
                        cout << "Warning for -v option: an unrecognized mode (i.e., value ";
                        cout << "out of [0,3]) was provided. The laconic mode (default mode) ";
                        cout << "will be used instead.\n" << endl;
                    }
                    break;
                case 'h':
                    displayUsage = true;
                    break;
                case 'i':
                    displayInfo = true;
                    break;
                default:
                    break;
            }
        }
    }
    catch(const std::logic_error &le)
    {
        cout << "Use -h or --help to get more details." << endl;
        return 1;
    }
    
    if(displayInfo || displayUsage)
    {
        if(displayInfo)
            printInfo();
        if(displayUsage)
            printUsage();
        return 0;
    }
    
    if(!found)
    {
        cout << "No target prefix or target file was provided. Use -h or --help to get more ";
        cout << "details." << endl;
        return 0;
    }
    
    /*
     * SETTING THE ENVIRONMENT
     *
     * Before listing target IPs, the initialization is completed by getting the local IP and the 
     * local subnet mask and creating a Environment object, which will be passed to other classes 
     * to be able to get all the current settings, which are either default values either values 
     * parsed in the parameters provided by the user. It also provides access to data structures 
     * other classes should be able to access.
     */

    if(localIPAddress.isUnset())
    {
        try
        {
            localIPAddress = InetAddress::getFirstLocalAddress();
        }
        catch(const InetAddressException &e)
        {
            cout << "Cannot obtain a valid local IP address for probing. ";
            cout << "Please check your connectivity." << endl;
            return 1;
        }
    }

    if(LANSubnetMask == 0)
    {
        try
        {
            LANSubnetMask = NetworkAddress::getLocalSubnetPrefixLengthByLocalAddress(localIPAddress);
        }
        catch(const InetAddressException &e)
        {
            cout << "Cannot obtain subnet mask of the local area network (LAN) .";
            cout << "Please check your connectivity." << endl;
            return 1;
        }
    }

    NetworkAddress LAN(localIPAddress, LANSubnetMask);
    
    /*
     * We determine now the label of the output files. Here, it is either provided by the user 
     * (via -l flag), either it is set to the current date (dd-mm-yyyy hh:mm:ss).
     */
    
    string newFileName = "";
    if(outputFileName.length() > 0)
        newFileName = outputFileName;
    else
        newFileName = getCurrentTimeStr();
    
    /*
     * Initialization of the Environment object (can be done like this, because it won't disappear 
     * before the end of main()).
     */
    
    Environment env(&cout, 
                    probingProtocol, 
                    localIPAddress, 
                    LAN, 
                    displayMode);
    
    // Parses config file, if provided, to get user's preferred parameters
    if(configFilePath.length() > 0)
    {
        ConfigFileParser parser(env);
        parser.parse(configFilePath);
    }
    
    /*
     * The code now checks if it can open a socket at all to properly advertise the user should 
     * use "sudo" or "su". Not putting this step would result in scheduling probing work 
     * (pre-scanning) and immediately trigger emergency stop (which should only occur when, after 
     * doing some probing work, software resources start lacking), which is not very elegant.
     */
    
    try
    {
        DirectICMPProber test(env.getAttentionMessage(), 
                              env.getTimeoutPeriod(), 
                              env.getProbeRegulatingPeriod(), 
                              DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER, 
                              DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER, 
                              DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE, 
                              DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE, 
                              false);
    }
    catch(const SocketException &e)
    {
        cout << "Unable to create sockets. Try running this program as a privileged user (for ";
        cout << "example, try with sudo)." << endl;
        return 1;
    }
    
    try
    {
        // Parses inputs and gets target lists
        TargetParser parser(env);
        parser.parseCommandLine(targetsStr);
        
        cout << "SAGE (Subnet AGgrEgation) v2.0 - Time at start: ";
        cout << getCurrentTimeStr() << "\n" << endl;
        
        // Announces that it will ignore LAN.
        if(parser.targetsEncompassLAN())
        {
            cout << "Target IPs encompass the LAN of the vantage point ("
                 << LAN.getSubnetPrefix() << "/" << (unsigned short) LAN.getPrefixLength() 
                 << "). IPs belonging to the LAN will be ignored.\n" << endl;
        }

        list<InetAddress> targetsPrescanning = parser.getTargetsPrescanning();
        
        // Stops if no target at all
        if(targetsPrescanning.size() == 0)
        {
            cout << "No target to probe." << endl;
            cout << "Use \"--help\" or \"-h\" parameter to reach help" << endl;
            return 1;
        }
        
        /*
         * STEP I: TARGET PRE-SCANNING
         *
         * Each address from the set of (re-ordered) target addresses are probed to check that 
         * they are live IPs.
         */
        
        TargetPrescanner prescanner(env);
        
        cout << "--- Start of target pre-scanning ---" << endl;
        timeval prescanningStart, prescanningEnd;
        gettimeofday(&prescanningStart, NULL);
        
        prescanner.run(targetsPrescanning);
        
        IPLookUpTable *IPDict = env.getIPDictionary();
        
        cout << "--- End of target pre-scanning (" << getCurrentTimeStr() << ") ---" << endl;
        gettimeofday(&prescanningEnd, NULL);
        unsigned long prescanningElapsed = prescanningEnd.tv_sec - prescanningStart.tv_sec;
        double successRate = ((double) env.getTotalSuccessfulProbes() / (double) env.getTotalProbes()) * 100;
        size_t nbResponsiveIPs = IPDict->getTotalIPs();
        cout << "Elapsed time: " << elapsedTimeStr(prescanningElapsed) << endl;
        cout << "Total amount of probes: " << env.getTotalProbes() << endl;
        cout << "Total amount of successful probes: " << env.getTotalSuccessfulProbes();
        cout << " (" << successRate << "%)" << endl;
        cout << "Total amount of discovered responsive IPs: " << nbResponsiveIPs << "\n" << endl;
        env.resetProbeAmounts();
        
        if(nbResponsiveIPs == 0)
        {
            cout << "Could not discover any responsive IP. Program will halt now." << endl;
            return 0;
        }
        
        IPDict->outputDictionary(newFileName + ".ips");
        cout << "Filtered target IPs have been saved in an output file " << newFileName;
        cout << ".ips (temporar)." << endl;
        
        /*
         * STEP II: TARGET SCANNING
         *
         * IPs that were responsive during pre-scanning are probed again in order to estimate 
         * their respective distance TTL-wise and find their trails. To speed-up the operation, 
         * multiple threads are used (see the TargetScanner class) and each thread probes 
         * consecutive IPs, using the estimated TTL of the previous IP as a way to reduce the 
         * probing work for the next one, relying on the hypothesis that consecutive IPs likely 
         * are located at the same distance (especially if they belong to the same subnet). At the 
         * end of the scanning, IPs are processed to detect problematic IPs (i.e. flickering, 
         * warping and echoing IPs) and attempt alias resolution on flickering IPs.
         */
        
        cout << "\n--- Start of target scanning ---" << endl;
        timeval scanningStart, scanningEnd;
        gettimeofday(&scanningStart, NULL);
        
        TargetScanner scanner(env);
        
        scanner.scan();
        scanner.finalize(); // Detects problematic IPs + aliases flickering IPs (if possible)
        
        cout << "--- End of target scanning (" << getCurrentTimeStr() << ") ---" << endl;
        gettimeofday(&scanningEnd, NULL);
        unsigned long scanningElapsed = scanningEnd.tv_sec - scanningStart.tv_sec;
        successRate = ((double) env.getTotalSuccessfulProbes() / (double) env.getTotalProbes()) * 100;
        cout << "Elapsed time: " << elapsedTimeStr(scanningElapsed) << endl;
        cout << "Total amount of probes: " << env.getTotalProbes() << endl;
        cout << "Total amount of successful probes: " << env.getTotalSuccessfulProbes();
        cout << " (" << successRate << "%)\n" << endl;
        env.resetProbeAmounts();
        
        IPDict->outputDictionary(newFileName + ".ips");
        cout << "Updated IP dictionary has been saved in the output file " << newFileName << ".ips." << endl;
        
        /*
         * STEP III: SUBNET INFERENCE
         *
         * All the data collected during the previous step is used to conduct offline subnet 
         * inference, processing IPs one by one and in order. A short post-processing phase also 
         * ensures subnets are as complete as possible.
         */
        
        cout << "\n--- Start of subnet inference ---" << endl;
        timeval inferenceStart, inferenceEnd;
        gettimeofday(&inferenceStart, NULL);
        
        cout << "Inferring subnets... " << std::flush;
        SubnetInferrer inferrer(env);
        inferrer.process();
        cout << "Done." << endl;
        
        cout << "Post-processing the discovered subnets... " << std::flush;
        SubnetPostProcessor postProcessor(env);
        postProcessor.process();
        cout << "Done." << endl;
        
        cout << "--- End of subnet inference (" << getCurrentTimeStr() << ") ---" << endl;
        gettimeofday(&inferenceEnd, NULL);
        unsigned long inferenceElapsed = inferenceEnd.tv_sec - inferenceStart.tv_sec;
        successRate = ((double) env.getTotalSuccessfulProbes() / (double) env.getTotalProbes()) * 100;
        cout << "Elapsed time: " << elapsedTimeStr(inferenceElapsed) << "\n" << endl;
        
        if(env.getNbSubnets() > 0)
        {
            env.outputSubnets(newFileName + ".subnets");
            cout << "Inferred subnets has been saved in the output file " << newFileName << ".subnets." << endl;
        }
        else
        {
            cout << "No subnet could be inferred." << endl;
        }
        
        /*
         * A save of the alias resolution hints/fingerprints is done now (if relevant), along with 
         * a save of the aliases discovered so far.
         */
        
        if(IPDict->hasAliasResolutionData())
        {
            IPDict->outputAliasHints(newFileName + ".hints");
            cout << "Alias hints (subnet inference) have been saved in the output file " << newFileName << ".hints." << endl;
            IPDict->outputFingerprints(newFileName + ".fingerprints");
            cout << "Fingerprints (subnet inference) have been saved in the output file " << newFileName << ".fingerprints." << endl;
            
            // This method will also write on the console to advertise the final output file name
            env.outputAliases(newFileName);
        }
        
        // If there's no subnet to work with, the program stops running here.
        if(env.getNbSubnets() == 0)
            return 0;
        
        /*
         * STEP IV: NEIGHBORHOOD INFERENCE
         *
         * Using the trails towards pivot IPs of all subnets, neighborhoods (i.e. network places 
         * surrounded by subnets located at at most one hop away from each other) are inferred. 
         * Prior to the neighborhood inference itself, the tool makes a census of all IPs that 
         * will be identifying neighborhoods (i.e., IPs from direct trails towards pivot IPs of 
         * each subnet) then starts a short (partial) traceroute phase to collect additional data 
         * on the pivot interfaces within subnets. This additional data is used to later infer how 
         * neighborhoods are located w.r.t. each others, using the notion of "peer". In WISE/SAGE 
         * terminology, a peer is an IP identifying a neighborhood which appears just before the 
         * trail(s) identifying another neighborhood. Therefore, both neighborhoods are located 
         * next to each other in the topology.
         *
         * The knowledge of both neighborhoods and peers is then used to build a neighborhood 
         * graph via the "TopologyInferrer" class. This construction consists itself in several 
         * stages (see TopologyInferrer.cpp for details), one of them involving alias resolution, 
         * in order to properly identify the final vertices (initial neighborhoods might be part 
         * of larger neighborhoods; this is disambiguated via alias resolution of peer IPs) and 
         * how they are connected together.
         */
        
        cout << "\n--- Start of neighborhood inference ---" << endl;
        timeval buildingStart, buildingEnd;
        gettimeofday(&buildingStart, NULL);
        
        PeerScanner peerScanner(env);
        peerScanner.scan(); // Makes the census mentioned above + partial traceroute probing
        
        AliasHints::moveStage(); // Neighborhood inference alias hints need to be distinguished
        
        TopologyInferrer topo(env);
        Graph *g = topo.infer(); // Infers neighborhoods and builds a graph
        
        Mariner mariner(env);
        mariner.visit(g); // "mariner" is used later to output stuff and to clean memory
        
        cout << "--- End of neighborhood inference (" << getCurrentTimeStr() << ") ---" << endl;
        gettimeofday(&buildingEnd, NULL);
        unsigned long buildingElapsed = buildingEnd.tv_sec - buildingStart.tv_sec;
        successRate = ((double) env.getTotalSuccessfulProbes() / (double) env.getTotalProbes()) * 100;
        cout << "Elapsed time: " << elapsedTimeStr(buildingElapsed) << endl;
        cout << "Total amount of probes: " << env.getTotalProbes() << endl;
        cout << "Total amount of successful probes: " << env.getTotalSuccessfulProbes();
        cout << " (" << successRate << "%)\n" << endl;
        env.resetProbeAmounts();
        
        IPDict->outputDictionary(newFileName + ".ips");
        cout << "Updated IP dictionary has been saved in the output file " << newFileName << ".ips." << endl;
        
        if(IPDict->hasAliasResolutionData())
        {
            IPDict->outputAliasHints(newFileName + ".hints");
            cout << "Alias hints (up to graph building) have been saved in the output file " << newFileName << ".hints." << endl;
            IPDict->outputFingerprints(newFileName + ".fingerprints");
            cout << "Fingerprints (up to graph building) have been saved in the output file " << newFileName << ".fingerprints." << endl;
            
            // This method will also write on the console to advertise the final output file name
            env.outputAliases(newFileName, AliasSet::GRAPH_BUILDING, false);
        }
        
        peerScanner.output(newFileName + ".peers");
        cout << "Additional traceroute data has been saved in the output file " << newFileName << ".peers." << endl;
        mariner.outputNeighborhoods(newFileName + ".neighborhoods");
        cout << "Neighborhoods have been saved in the output file " << newFileName << ".neighborhoods." << endl;
        mariner.outputGraph(newFileName + ".graph");
        cout << "Network graph been saved in the output file " << newFileName << ".graph." << endl;
        
        /*
         * STEP V: FULL ALIAS RESOLUTION
         *
         * After the construction of the graph, SAGE visits it with several "voyagers" - classes 
         * which apply a specific treatment to the graph. In particular, the "Galileo" voyager 
         * visits all neighborhoods to perform the final alias resolution - discovery of aliases 
         * between router interfaces (appearing in trails, identifying neighborhoods) and the 
         * contra-pivot interfaces of the neighborhing subnets. The "Cassini" voyager also 
         * operates afterwards, as it is designed to compute various metrics on the graph, which 
         * includes metrics on the discovered aliases.
         */
        
        cout << "\n--- Start of full alias resolution ---" << endl;
        timeval aliasResolutionStart, aliasResolutionEnd;
        gettimeofday(&aliasResolutionStart, NULL);
        
        AliasHints::moveStage(); // Hints collected by Galileo will have a different stage too
        
        Galileo galileo(env);
        galileo.visit(g); // Aliases are written in an output file a few instructions below
        
        Cassini cassini(env);
        cassini.visit(g); // Metrics are written in an output file a few instructions below
        
        cout << "--- End of full alias resolution (" << getCurrentTimeStr() << ") ---" << endl;
        gettimeofday(&aliasResolutionEnd, NULL);
        unsigned long aliasingElapsed = aliasResolutionEnd.tv_sec - aliasResolutionStart.tv_sec;
        successRate = ((double) env.getTotalSuccessfulProbes() / (double) env.getTotalProbes()) * 100;
        cout << "Elapsed time: " << elapsedTimeStr(aliasingElapsed) << endl;
        cout << "Total amount of probes: " << env.getTotalProbes() << endl;
        cout << "Total amount of successful probes: " << env.getTotalSuccessfulProbes();
        cout << " (" << successRate << "%)\n" << endl;
        env.resetProbeAmounts();
        
        // Final output files
        IPDict->outputAliasHints(newFileName + ".hints");
        cout << "Final alias hints have been saved in the output file " << newFileName << ".hints." << endl;
        IPDict->outputFingerprints(newFileName + ".fingerprints");
        cout << "Final fingerprints have been saved in the output file " << newFileName << ".fingerprints." << endl;
        galileo.figaro(newFileName + ".aliases-f");
        cout << "Final aliases been saved in the output file " << newFileName << ".aliases-f." << endl;
        cassini.outputMetrics(newFileName + ".metrics");
        cout << "Network graph metrics been saved in the output file " << newFileName << ".metrics." << endl;
        
        // Frees the graph from the memory
        mariner.cleanVertices();
        delete g;
    }
    catch(const StopException &e)
    {
        cout << "Halting now." << endl;
        return 1;
    }
    
    return 0;
}
