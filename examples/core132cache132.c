/*******************************************************************************
*  ______                            ______                 _
* (_____ \                          |  ___ \               | |
*  _____) )___   _ _ _   ____   ___ | | _ | |  ___   ____  | |  _  ____  _   _
* |  ____// _ \ | | | | / _  ) / __)| || || | / _ \ |  _ \ | | / )/ _  )| | | |
* | |    | |_| || | | |( (/ / | |   | || || || |_| || | | || |< (( (/ / | |_| |
* |_|     \___/  \____| \____)|_|   |_||_||_| \___/ |_| |_||_| \_)\____) \__  |
*                                                                       (____/
* Copyright (C) 2021 Ivan Dimkovic. All rights reserved.
*
* All trademarks, logos and brand names are the property of their respective
* owners. All company, product and service names used are for identification
* purposes only. Use of these names, trademarks and brands does not imply
* endorsement.
*
* SPDX-License-Identifier: Apache-2.0
* Full text of the license is available in project root directory (LICENSE)
*
* WARNING: This code is a proof of concept for educative purposes. It can
* modify internal computer configuration parameters and cause malfunctions or
* even permanent damage. It has been tested on a limited range of target CPUs
* and has minimal built-in failsafe mechanisms, thus making it unsuitable for
* recommended use by users not skilled in the art. Use it at your own risk.
*
*******************************************************************************/

#include "Platform.h"

/*******************************************************************************
 *                   !!! WARNING - ACHTUNG - VNIMANIE !!!
 *     PROCEED IF YOU ARE AN EXPERT ONLY - DO NOT RANDOMLY "PUSH BUTTONS"!
 * 
 * You need to roll-your-own EFI binary by compiling this code. Here are some
 * basic steps to keep in mind before compiling and running:
 *
 * 1.   This POC was tested on mobile CML-H (Comet Lake) CPUs. It >should< work 
 *      on RKL/TGL/ICL, but it was not tested, so YMMV. It could also run on KBL
 *      /CFL and SKL platforms without V/F point overrides, but none of those 
 *      systems are tested as of now.
 *
 *      In any case, you must ensure that the programming (MSRs, etc.) are sane
 *      for your system. What is good for one system is not necessarily working
 *      on other. Settings used here are for example purposes only.
 *
 * 2.   Please review this file, adjust values appropriately (and check MSRs)
 *
 * 3.   Never test on production systems! Please consider using debugging rig 
 *      testing.  While it is not expected to end up with a brick whatever you 
 *      enter, it is always recommended to have either a board with brick-proof 
 *      emergency firmware recovery (that works) OR a hardware flash programmer
 * 
 * 4.   If you plan to use the settings as sticky in production, please make 
 *      sure you tested the system under stress to confirm stability!
 ******************************************************************************/

/*******************************************************************************
 * Global Application Settings
 ******************************************************************************/

///
/// Enable / Disable "Emergency Exit"
/// Enabling this option adds 3 second delay with possibility to abort
/// by pressing ESC key. Disable it only if you are absolutely sure that your
/// configuration is stable and you want to save 3 seconds of boot time
///

UINT8 gEmergencyExit = 1;

///
/// Enable safer hardware probing (default: 1)
/// If PowerMonkey.efi cannot start but hangs your system, try disabling this 
/// flag as its mechanism is involving low-level hooking of system interrupts
/// and could trip some paranoid firmware
///

UINT8 gEnableSaferAsm = 1;

///
/// Disable UEFI watchdog timer
/// Will be useful once stress testing is fully implemented 
/// 

UINT8 gDisableFirwmareWDT = 0;

///
/// SELF TEST (STRESS TEST) - MAX RUNS
/// Set this to a value higher than 0 to enable stress self-testing
/// Typical values: 0 (no stress testing); 10 (very short); 100+ (longer)

UINT64 gSelfTestMaxRuns = 0; /// DO NOT ENABLE YET (WIP)

/*******************************************************************************
 * ApplyComputerOwnersPolicy()
 * 
 * This is where it is done. With no external config. Party like it's 1979.
 * 
 * NOTE: Voltage offsets are limited to +/- 250 mV. Please see VoltTables.c
 * if you wish to do more dangerous volt-mod, you will need to adapt the code
 ******************************************************************************/

/*******************************************************************************
 * Settings here are for a particular Core i7 10850H (mine) and need to be
 * adjusted for your CPU(s). Please do not just "run" the code but check your
 * values (e.g. run ThrottleStop if you use Windows) and change accordingly.
 * What you see below is an EXAMPLE ONLY.
 ******************************************************************************/

VOID ApplyComputerOwnersPolicy(IN PLATFORM* sys)
{
  //
  // We will loop through all packages (processor sockets)
  // and override (program) things we like changed.

  for (UINTN pidx = 0; pidx < sys->PkgCnt; pidx++) {
    
    PACKAGE* pk = sys->packages + pidx;

    /////////////////////////////////////
    /// Voltage / Frequency Overrides ///
    /////////////////////////////////////

    //
    // Select which domains are to be programmed
    
    pk->Program_VF_Overrides[IACORE] =    1;    // Enable programming of VF
                                                // Overrides for IA Cores

    pk->Program_VF_Overrides[RING] =      1;    // Enable programming of VF
                                                // Overrides for Ring / Cache

    pk->Program_VF_Overrides[UNCORE] =    0;    // Enable programming of VF
                                                // Overrides for Uncore (SA)

    pk->Program_VF_Overrides[GTSLICE] =   0;    // Enable programming of VF
                                                // Overrides for GT Slice

    pk->Program_VF_Overrides[GTUNSLICE] = 0;    // Enable programming of VF
                                                // Overrides for GT Unslice

    ///
    /// V/F OVERRIDES FOR DOMAIN: CORE
    ///
    
    pk->Domain[IACORE].VoltMode = V_IPOLATIVE;  // V_IPOLATIVE = Interpolate
                                                // V_OVERRIDE =  Override

    pk->Domain[IACORE].TargetVolts =  0;        // in mV (absolute)
    pk->Domain[IACORE].OffsetVolts = -132;      // in mV (negative = undervolt)

    ///
    /// V/F OVERRIDES FOR DOMAIN: RING
    ///

    // Note: some domains are sharing the same voltage plane! Check yours!
    // 
    // E.g.: for CML-H, IACORE (CPU cores) and RING (cache) share a common VR
    // if you don't program both linked domains to exactly the same voltage, 
    // CPU's pcode will take one (higher, which means less undervolt!) and
    // apply it to both domains - but without adjusting values submitted by the 
    // user so it appears everything went as planned. Some might believe they 
    // won the 'chip lottery' seeing their CPU seemingly undervolt to -250 mV 
    // or smth. while in reality pcode is doing exactly nothing because cache
    // is programmed to 0 mV offset. Don't be that guy (or girl)!

    pk->Domain[RING].VoltMode = V_IPOLATIVE;   // V_IPOLATIVE = Interpolate
                                               // V_OVERRIDE  = Override

    pk->Domain[RING].TargetVolts = 0;          // in mV (absolute)
    pk->Domain[RING].OffsetVolts = -132;       // in mV (negative = undervolt)


    ///
    /// V/F OVERRIDES FOR DOMAIN: SA
    ///
    
    // Add your adjustments here if needed    

    ///
    /// V/F OVERRIDES FOR DOMAIN: GT SLICE
    ///

    // Add your adjustments here if needed

    ///
    /// V/F OVERRIDES FOR DOMAIN: GT UNSLICE
    ///

    // Add your adjustments here if needed

    ///////////////////////////
    /// V/F Curve Adjustment //
    ///////////////////////////
    
    ///
    /// VF Curve Point (VF Point) Adjustment has been introduced in CML
    /// This feature does not work in older generations. At the moment, only
    /// "OC Unlocked" parts allow adjusting (or even just reading!) of VF
    /// curve points. You can program VF curve here, or use setting of "2"
    /// to just print the current VF curve. There are up to 15 VF points
    /// and you need to probe the CPU to see how many points your has and
    /// what are the frequencies (ratios) of each point.
    /// 
    /// You cannot modify the ratios (they are read-only) and the only allowed
    /// voltage format is "offset mode".
    ///
    /// VF Curve Adjustment is only supported by CORE and RING domains
    /// and for CML and RKL (at least) you must ensure that the same values
    /// are programmed for both domains! (they are sharing the same VR)

    pk->Program_VF_Points = 0;                      // 0 - Do not program
                                                    // 1 - Program
                                                    // 2 - Print current values
                                                    //     (2 does not program)

    ///
    /// Change to your CPU config!
    /// NOTE: VF Points here are ZERO INDEXED and can go from VP[0] to VP[14] !
    /// 
    /// Uncomment below block (lines 220/224)
/*
    pk->Domain[IACORE].vfPoint[0].OffsetVolts =
      pk->Domain[RING].vfPoint[0].OffsetVolts = 0;      // 800 MHz (Example)

    pk->Domain[IACORE].vfPoint[1].OffsetVolts =
      pk->Domain[RING].vfPoint[1].OffsetVolts = -50;    // 2500 MHz (Example)

    pk->Domain[IACORE].vfPoint[2].OffsetVolts =
      pk->Domain[RING].vfPoint[2].OffsetVolts = -100;   // 3500 MHz (Example)

    pk->Domain[IACORE].vfPoint[3].OffsetVolts =
      pk->Domain[RING].vfPoint[3].OffsetVolts = -130;   // 4300 MHz (Example)

    pk->Domain[IACORE].vfPoint[4].OffsetVolts =
      pk->Domain[RING].vfPoint[4].OffsetVolts = -110;   // 4800 MHz (Example)

    pk->Domain[IACORE].vfPoint[5].OffsetVolts =
      pk->Domain[RING].vfPoint[5].OffsetVolts = -100;   // 5100 MHz (Example)

    pk->Domain[IACORE].vfPoint[6].OffsetVolts =
      pk->Domain[RING].vfPoint[6].OffsetVolts = -100;   // 5200 MHz (Example)

    pk->Domain[IACORE].vfPoint[7].OffsetVolts =
      pk->Domain[RING].vfPoint[7].OffsetVolts = -100;   // 5200 MHz (Example)
 */
 
    /////////////
    // ICC Max //
    /////////////

    //
    // Note: check the capabilities of your CPU
    // Do not program too high value as damage might occur!

    // pk->Program_IccMax[RING] =      1;  // Enable IccMax override for Ring/$
    // pk->Program_IccMax[IACORE] =    1;  // Enable IccMax override for IA Cores
    // pk->Program_IccMax[UNCORE] =    0;  // Enable IccMax override for SA/Uncore
    // pk->Program_IccMax[GTSLICE] =   0;  // Enable IccMax override for GT Slice
    // pk->Program_IccMax[GTUNSLICE] = 0;  // Enable IccMax override for GT Unslice

    //
    // IccMax Values
    
    // pk->Domain[IACORE].IccMax = 
    //   pk->Domain[RING].IccMax = MAX_AMPS;      // 1/4 Amps Unit or MAX_AMPS

    ////////////////////
    /// Turbo Ratios ///
    ////////////////////
    
    // 
    // Enable this to program "max ratio" for all turbo core counts
    // (e.g. 1C, 2C, 4C, 8C, = use this ratio). Remove or set to 0
    // if you do not wish to set it

    // pk->ForcedRatioForAllCoreCounts = 51;

    /////////////////////
    /// Power Control ///
    /////////////////////
    
    // pk->ProgramPowerControl = 1;                // Enable programing of power
                                                // control knobs

    // pk->EnableEETurbo = 1;                      // Energy Efficient Turbo
                                                // (0=disable, 1=enable)

    // pk->EnableRaceToHalt = 1;                   // Race To Halt
                                                // (0=disable, 1=enable)

    ////////////////////
    /// Power Limits ///
    ////////////////////
    
    //
    // Modify settings below only if you believe your, e.g.notebook, is over
    // aggressively limiting power(e.g.in 'desk' mode on AC).Please note
    // that many notebooks are very nicely configured, or their cooling system
    // cannot take more heat.Also, just because some value is higher (say,
    // 135W instead of 68W), this doesn't mean you will end up with more 
    // performance, as the CPU might end up in throttle - fest due to thermal
    // overloading of the cooling system or just reaching CPU's fused limits.

    // If you want to do this properly, you need to set up a benchmark script
    // simulating your workloads and carefully tweak the PLx values until you
    // hit the best scores. New systems also have more flexibility, such as
    // "dual Tau boost" that add more control levels and possibly make your
    // system >actually< faster at the end.Once you are done with benchmarking
    // and testing, PowerMonkey will happily set those values for you before 
    // OS or Hypervisor kick in.

    // Note: MAX_POWAH constant will still respect maximum PLx as reported by
    // the firmware.Some firmware(e.g.mine) report 0 instead, in which case
    // one will end up with programmed values like "4095 Watts".If you do not
    // like the checks done, you need to modify the code.But in that case, the
    // firmware might still refuse to program too high of a setting.

    // Bonus special : once there used to be one set of limits, you could
    // configure using an MSR.Then, vendors started introducing moar PLs(pkg,
    // platform, etc.) and also more ways to program(MSR, MMIO).Then, someone
    // had the great idea to use more than one record(say, MSRand MMIO) and
    // combine them into a more flexible config space.Of course, OEMs also
    // need more control(PL3, PL4, ...) and in the case of a notebook, say,
    // you wish OEM to have the final say so that EC can override anything with
    // emergency values.That would save you from melting your notebook, or
    // worse.There is no guide here to deal with this - please consult forums
    // if you do not have access to the relevant BWG doc.Or ignore all this
    // noise and set everything to MAX_POWAH as many will promptly do).
    // 
    // MAX_POWAH is a dummy value indicating to the policy updater that the
    // user prefers to set the value to "maximum allowed range". Useful for 
    // isolating sources of throttling. But it will not magically allow your
    // 13"-thin-and-light notebook to generate 500W of heat. Finding optimal
    // PLx values is much better idea. MAX_POWAH can also be dangerous when
    // platform does not have PECI-enforced failsafe (one could configure PLs
    // to be unsafe electrically and damage the system physically or worse)

    //
    // Select which parameters you want to program
    // If ProgramXXX flag is set to 0, entire knob will not be touched
    // This is also true for lock bits (if you wish to lock PL1, you need
    // to set ProgramPL12xx to 1)

    // pk->ProgramPL12_MSR =  1;               // Program MSR PL1/2
    // pk->ProgramPL12_MMIO = 1;               // Program MMIO PL1/2
    // pk->ProgramPL12_PSys = 1;               // Program Platform PLs
    // pk->ProgramPL3 = 1;                     // Program PL3
    // pk->ProgramPL4 = 1;                     // Program PL4
    // pk->ProgramPP0 = 1;                     // Program PP0

    //////////////
    // Settings //
    //////////////

    //
    // Configurable TDP (cTDP)

    // pk->MaxCTDPLevel =  0;                  // 0 = disables cTDP
    // pk->TdpControLock = 1;                  // Locks TDP config

    //
    // Package PL1/PL2 (MSR)
    
    // pk->EnableMsrPkgPL1 = 1;                // Enable PL1 
    // pk->EnableMsrPkgPL2 = 1;                // Enable PL2
    // pk->MsrPkgPL1_Power = MAX_POWAH;        // PL1 in mW or MAX_POWAH
    // pk->MsrPkgPL2_Power = MAX_POWAH;        // PL2 in mW or MAX_POWAH
    // pk->MsrPkgPL_Time =   MAX_POWAH;        // Tau in ms or MAX_POWAH
    // pk->ClampMsrPkgPL =   1;                // Allow clamping
    // pk->LockMsrPkgPL12 =  1;                // Lock after programming

    //
    // Package PL1/PL2 (MMIO)

    // pk->EnableMmioPkgPL1 = 1;               // Enable MMIO PL1
    // pk->EnableMmioPkgPL2 = 1;               // Enable MMIO PL2
    // pk->MmioPkgPL1_Power = MAX_POWAH;       // MMIO PL1 in mW or MAX_POWAH
    // pk->MmioPkgPL2_Power = MAX_POWAH;       // MMIO PL2 in mW or MAX_POWAH
    // pk->MmioPkgPL_Time   = MAX_POWAH;       // Tau in ms or MAX_POWAH
    // pk->ClampMmioPkgPL =   1;               // Allow clamping
    // pk->LockMmioPkgPL12 =  1;               // Lock after programming

    //
    // Platform (PSys) PL1/PL2 

    // pk->EnablePlatformPL1 = 1;              // Enable PSys PL1 
    // pk->EnablePlatformPL2 = 1;              // Enable PSys PL2
    // pk->PlatformPL1_Power = MAX_POWAH;      // PSys PL1 in mW or MAX_POWAH
    // pk->PlatformPL_Time =   MAX_POWAH;      // RAW VALUE 0-127 (or MAX_POWAH)
    // pk->PlatformPL2_Power = MAX_POWAH;      // PSys PL2 in mW or MAX_POWAH
    // pk->ClampPlatformPL =   1;              // Allow clamping
    // pk->LockPlatformPL =    1;              // Lock after programming

    //
    // Package PL3

    // pk->EnableMsrPkgPL3 = 1;                // Enable MSR PL4
    // pk->MsrPkgPL3_Power = MAX_POWAH;        // PL3 in mW or MAX_POWAH
    // pk->MsrPkgPL3_Time  = MAX_POWAH;        // Tau in ms or MAX_POWAH
    // pk->LockMsrPkgPL3 =   1;                // Lock PL1 after programming

    //
    // Package PL4
    //
    // Note: I do not recommend pushing this setting up. It sits near the
    // end of possible strategies to avoid power escalation. Setting it too
    // high might bring your system so out of spec that physical damage can
    // occur. While some systems have additional protections, one should not
    // test this damage can be irreversible. I'd consider changing this only
    // if I have strong reason to suspect it is too low (seeing CPU reporting
    // too many throttling events)

    // pk->EnableMsrPkgPL4 = 1;                // Enable PL4
    // pk->MsrPkgPL4_Current = MAX_POWAH;      // MSR PL4 Amperes or MAX_POWAH
    // pk->LockMsrPkgPL4 = 1;                  // Lock PL4 after programming

    //
    // PP0

    // pk->EnableMsrPkgPP0 = 1;                // Enable MSR PP0
    // pk->MsrPkgPP0_Power = MAX_POWAH;        // Power in mW or MAX_POWAH
    // pk->MsrPkgPP0_Time =  MAX_POWAH;        // Time in ms or MAX_POWAH
    // pk->ClampMsrPP0 =     1;                // Allow clamping
    // pk->LockMsrPP0 =      1;                // Lock after programming
  }
}
