This is modified from msdn example code, add several lines in code to make it support force connecting the specifi bssid.


1.
Client.exe ?
Use "help xyz" to show the description of command xyz.
        DeleteProfile(dp)
        Connect(conn)
        Disconnect(dc)
        GetInterfaceState(gs)
        help(?)

Client.exe help conn
Command: Connect(conn)
Description: Connect to a wireless network using a saved profile.
Usage: Connect(conn) <SSID> <Profile name> <Profile path> <bssid> <EapUserProfile path(optional)>

client.exe help dc
Command: Disconnect(dc)
Description: Disconnect from the current network.
Usage: Disconnect(dc)

Client.exe help dp
Command: DeleteProfile(dp)
Description: Delete a saved profile.
Usage: DeleteProfile(dp) <profile name>

Client.exe help gs
Command: GetInterfaceState(gs)
Description: Get the interface state
Usage: GetInterfaceState(gs)

2. Connect

client.exe conn <SSID> <Profile name> <Profile path> <bssid> <EapUserProfile path(optional)>

example:
/cygdrive/c/client1/Client.exe conn fd1_PeapMschapv2 fd1_PeapMschapv2 c:/client1/fd1_PeapMschapv2.xml 00246c3306ca c:/client1/fd1_PeapUser.xml
Command "conn" completed successfully.

wpa-psk: client.exe conn fd1 fd1 c:\fd1_wpa_psk.xml 001122334455
peap-gtc: client.exe conn fd1 fd1 c:\fd1_peap_gtc.xml 001122334455
peap-mschapv2: client.exe conn fd1 fd1 c:\fd1_peap_mschapv2.xml 001122334455 c:\fd1_peap_user.xml
tls: client.exe fd1 fd1 c:\fd1_tls.xml 001122334455 c:\fd1_tls_cert.xml
ttls: client.exe fd1 fd1 c:\fd1_ttls.xml 001122334455


3. Disconnect
client.exe dc

4. Get interface state
client.exe gs
example:
/cygdrive/c/client1/Client.exe gs
 "associating"
 Command "gs" completed successfully.
/cygdrive/c/client1/Client.exe gs
 "connected"
Command "gs" completed successfully.

5. Delete the profile 
client.exe dp <profile name>

example:
client.exe dp fd1
Command "dp" completed successfully.

if no this profile, will print:

Client.exe dp fd1_PeapMschapv2
Got error 1168 for command "dp"



Limitations:
1. Just support win7, please don't use it on other systems!!!
2. Because win7 itself does not support ttls, it will be provided by the network adapter driver provider,
  so if you want to change the ttls profile, you can use netsh export the xml file from profile.
  
  for example, I used the adapter which was provided by intel, I want to connect to the ssid fd1_ttls.
  I will use intel proset to connect fd1_ttls first, then I can find the profile from
  Control Panel\Network and Internet\Manage Wireless Networks
  I will use the command to export the xml file which I want.
  netsh wlan export profile name="fd1_ttls" folder=c:\ key=clear
  then you can get the xml file from c:\
  
 3. The parameters of the tool "client.exe" should contain at least 1 bssid and just 1 bssid.
