# VISA Secure Access Policy checker tool

# Introduction
This tool is intended to inspect the VISA (Visualization of Internal Signals Architecture, see [\[Intel VISA Through the Rabbit Hole\]][1]) Secure Access Policy on various Intel's platforms (it is developed to be run in Windows OS). As we found the VISA technology has a build-in mechanism allowing to bypass implemented protections, i.e. a kind of backdoor. This mechanism is implemented at the level of the VISA serial bus using to setup VISA multiplexers and consists from a special bit of the bus protocol called Secure Access Bit. This "secure" access means the bypass of the protections applied by various DFX Secure Policies (see [\[Intel Debug Technology Whitepaper\]][2]). Any SoCs/PCH/CPU hardware device (IP unit) can be allowed to engage the Secure Access Bit by the special access control policy in Intel Trace Hub device programmed at RTL level. We developed the tool to research the set of the hardware in modern Intel platforms which has the ability to bypass the DFX protections. One of the IPs engaging the Secure Access Bit on most platforms is Intel CSME.

# Usage
To read the Intel Trace Hub (a.k.a. North Peak, NPK) Secure Access Policy the tool reads the policy's registers at 0xffc00-0xffc20 offsets in NPK CSR (Control and Status Registers) BAR (#0 at 0x10-0x18 in NPK PCI CFG space). Because the access to NPK CSR implies the access to physical memory the tool relies on the signed kernel-mode Windows driver rwdrv.sys provided with [\[RWEverything utility\]][3]. For successful completion, visasecacc_test tool must be run under rights of Administrator Windows user and after the RW Everything was run at least once (so, rdwrv.sys exists in system/drivers folder).
We created a file (results.txt) intended to accumulate all unique results from runs on all possible Intel
 platforms. So, the run of the tool on the Windows OS has the following form:

```
 .\visasecacc_test\x64\Release\visasecacc_test.exe >> results.txt
 ```

After the github merge request we will merge your information if it is from platforms that don't already exist in results.txt.

[1]: https://i.blackhat.com/asia-19/Thu-March-28/bh-asia-Goryachy-Ermolov-Intel-Visa-Through-the-Rabbit-Hole.pdf
[2]: https://software.intel.com/content/www/us/en/develop/articles/software-security-guidance/secure-coding/intel-debug-technology.html
[3]: http://rweverything.com/
