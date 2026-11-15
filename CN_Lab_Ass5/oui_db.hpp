#ifndef OUI_DB_HPP
#define OUI_DB_HPP

#include <string>
#include <map>
#include <algorithm>

class OuiDatabase {
private:
    std::map<std::string, std::string> oui_table;

    void init() {
        // Standard OUI mappings including user environment and major networking vendors
        oui_table["dc:4b:a1"] = "Intel Corporate";
        oui_table["f4:ca:e7"] = "Shenzhen Bilian Electronic Co., Ltd.";
        oui_table["00:0c:29"] = "VMware, Inc.";
        oui_table["08:00:27"] = "Oracle VirtualBox";
        oui_table["52:54:00"] = "QEMU / KVM Virtual NIC";
        oui_table["00:15:5d"] = "Microsoft Hyper-V";
        oui_table["00:50:56"] = "VMware ESXi";
        oui_table["00:1a:2b"] = "Cisco Systems, Inc.";
        oui_table["00:00:0c"] = "Cisco Systems, Inc.";
        oui_table["00:1e:67"] = "Intel Corporate";
        oui_table["f8:75:a4"] = "Apple, Inc.";
        oui_table["3c:22:fb"] = "Apple, Inc.";
        oui_table["e4:5f:01"] = "Raspberry Pi Foundation";
        oui_table["b8:27:eb"] = "Raspberry Pi Foundation";
        oui_table["d8:3a:dd"] = "Raspberry Pi Trading Ltd";
        oui_table["70:4f:57"] = "Realtek Semiconductor Corp.";
        oui_table["00:e0:4c"] = "Realtek Semiconductor Corp.";
        oui_table["50:c7:bf"] = "TP-Link Technologies Co., Ltd.";
        oui_table["14:cc:20"] = "TP-Link Technologies Co., Ltd.";
        oui_table["ec:08:6b"] = "TP-Link Corporation Limited";
        oui_table["00:11:32"] = "Synology Incorporated";
        oui_table["00:24:8c"] = "ASUSTek Computer Inc.";
        oui_table["04:d9:f5"] = "ASUSTek Computer Inc.";
        oui_table["ac:84:c6"] = "Dell Inc.";
        oui_table["b0:4e:26"] = "TP-Link Corporation Limited";
        oui_table["ff:ff:ff"] = "Broadcast";
    }

public:
    OuiDatabase() {
        init();
    }

    std::string lookup(const std::string& mac) const {
        if (mac.length() < 8) return "Unknown";
        std::string prefix = mac.substr(0, 8);
        std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);
        
        auto it = oui_table.find(prefix);
        if (it != oui_table.end()) {
            return it->second;
        }
        return "Unknown Manufacturer (" + prefix + ")";
    }

    std::string get_oui_hex(const std::string& mac) const {
        if (mac.length() < 8) return "";
        std::string prefix = mac.substr(0, 8);
        std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
        return prefix;
    }
};

#endif // OUI_DB_HPP

