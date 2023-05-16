# VISA Secure Access Policy checker tool

# Introduction
This tool is intended to inspect the VISA (Visualization of Internal Signals Architecture, see [\[Intel VISA Through the Rabbit Hole\]][1]) Secure Access Policy on various Intel's platforms (it is developed to be run in Windows OS). As we found the VISA technology has a build-in mechanism allowing to bypass implemented protections, i.e. a kind of backdoor. This mechanism is implemented at the level of the VISA serial bus using to setup VISA multiplexers and consists from a special bit of the bus protocol called Secure Access Bit. This "secure" access means the bypass of the protections applied by various DFX Secure Policies (see [\[Intel Debug Technology Whitepaper\]][2]). Any SoCs/PCH/CPU hardware device (IP unit) can be allowed to engage the Secure Access Bit by the special access control policy in Intel Trace Hub device programmed at RTL level. We developed the tool to research the set of the hardware in modern Intel platforms which has the ability to bypass the DFX protections. One of the IPs engaging the Secure Access Bit on most platforms is Intel CSME.  
We also developed the Python's script (**visasecacc.py**) to accomplish the same tasks but in OpenIPC command line console while perfoming JTAG debugging of the target platform. You can simply copy/past all the file content into IPC command line and call **print_visa_sec_access_policy** function in the console. This function can be called just after CPU cores halt on Reset Break. This can be usefull on some platforms where UEFI/BIOS Setup doesn't allow turning on Intel Trace Hub and permanently disables it.

# Usage
To read the Intel Trace Hub (a.k.a. North Peak, NPK) Secure Access Policy the tool reads the policy's registers at 0xffc00-0xffc20 offsets in NPK CSR (Control and Status Registers) BAR (#0 at 0x10-0x18 in NPK PCI CFG space). Because the access to NPK CSR implies the access to physical memory the tool relies on the signed kernel-mode Windows driver rwdrv.sys provided with [\[RWEverything utility\]][3]. For successful completion, visasecacc_test tool must be run under rights of Administrator Windows user and after the RW Everything was run at least once (so, rdwrv.sys exists in system/drivers folder).
We created a file (results.txt) intended to accumulate all unique results from runs on all possible Intel
 platforms. So, the run of the tool on the Windows OS has the following form:

```
 .\visasecacc_test\x64\Release\visasecacc_test.exe >> results.txt
 ```

or in OpenIPC environment:

```
>>> print_visa_sec_access_policy()
 ```

The sample output can be as following:

```
-----------------------------------------------------------------
CPUID: 0x706a1: HOST DID: 0x31f0: PCH DID: 0x31e8:
REVISION ID: 0x03: MANUFACTURER ID: 0x01000f1c: PLATFORM ID: 0x0
NPK BDF: 0:0:2
NPK CSR BAR: 0xfef00000
NPK VISA Secure Policy: SCP: 0x0000000000000000: SWP: 0x0000000000000000: SRP: 0xffffffffffffffff
 ```

After the github merge request we will merge your information if it is from platforms that don't already exist in results.txt.

[1]: https://i.blackhat.com/asia-19/Thu-March-28/bh-asia-Goryachy-Ermolov-Intel-Visa-Through-the-Rabbit-Hole.pdf
[2]: https://software.intel.com/content/www/us/en/develop/articles/software-security-guidance/secure-coding/intel-debug-technology.html
[3]: http://rweverything.com/
