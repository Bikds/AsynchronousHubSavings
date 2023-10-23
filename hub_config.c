#include <stdio.h>
#include <math.h>
#include "hub_config.h"

int main()
{
 
  int up_in_width, up_out_width, up_freq;
  //UFP width in (upstream facing - towards host) and
  //out (downstream facing - towards device) and
  //freq (note 5 GT/s changed to 4 GT/s)
  int num_dp; // number of downstream ports
  int dp_usb[15];// Type of USB:1, 2, 3, 4
  int dp_in_width[15], dp_out_width[15], dp_freq[15];
  // Only meaningful for USB3 and above; else 0
  double dp_up_bw_demand[15], dp_dn_bw_demand[15];
                 // Actual B/W demand in each direction
  double net_up_demand=0, net_dn_demand=0; // represents the actual b/w demand
                    // per direction from all the DPs (util * raw b/w)
  double up_in_bw,up_out_bw;  //total b/w available per direction in UP
  double dp_in_raw_bw_aggr,dp_out_raw_bw_aggr; // Raw B/W total
  //of all DPs in each direction
  double raw_bw_allot_in[15],raw_bw_allot_out[15];
          // Raw B/W allottment if each DP requested full B/W
  double dp_actual_bw_allot_up[15],dp_actual_bw_allot_dn[15];
  // Actual b/w allocated to each DP by the host/hub depending on their
  // demand and their fair share of b/w allocation
  //
 
  //
  // The following are used during the b/w allocation
  //
  double aggr_bw_allotted_up=0,aggr_bw_allotted_dn=0;
     // Used during b/w allocation to count how much has been allotted this far
  short int dp_bw_demand_met_up[15],dp_bw_demand_met_dn[15];
  // The above is used to denote whether b/w demand is fully met or not
  // The following two variables are the remainder
  // Following two variable: For each DP/direction, we count if the
  // b/w is allocated or not after first pass - if yes, that DP's ratio is
  // made 0 - these represent the remaining ratios added - so in the second
  // pass we do the proportionate b/w distribution to those ports whose
  // full demand can not be met
  double raw_bw_allot_remain_after_first_pass_up=0; 
  double raw_bw_allot_remain_after_first_pass_dn=0;
  // These are the actual b/w remaining after first pass
  double bw_remain_up_after_first_pass;
  double bw_remain_dn_after_first_pass;
  //
  // Actual utilization after the 2-pass b/w allocation
  double dp_util_up[15],dp_util_dn[15],dp_util_link[15];
  double up_util_up, up_util_dn, up_util_link;
  //
  double time_tot;
  double time_in_u0_or_entry_exit_u1, time_in_u0_or_entry_exit_u2, time_in_u0_or_entry_exit_u3;
  double time_out_u0_or_entry_exit_u1, time_out_u0_or_entry_exit_u2, time_out_u0_or_entry_exit_u3;
  double frac_in_u1, frac_out_u1, frac_in_u2, frac_out_u2, frac_in_u3, frac_out_u3;
 
  double power_savings_port_in_ux,power_savings_port_out_ux;
  double total_power_savings_hub_u1;
  double total_power_savings_hub_u2;
  double total_power_savings_hub_u3;
 
 
  // double frac_up_bw_alloc_to_dp_up[15],frac_up_bw_alloc_to_dp_dn[15];
  // The above represents
  int i,j;
  //
  double raw_bw_allot_tot_in=0;
  double raw_bw_allot_tot_out=0;
  raw_bw_allot_tot_out=0;

  int USB3_ASYM_L1_SUPPORT=0;

  FILE *fp1;
 
  printf("-----------------------------------\n");
  printf("Enter the Hub configuration in `hub_config.in' file\n");
  printf("Expected format: Upstream Port (UP) width in, width out, frequency for USB3+ (expected to have USB2) \n \t followed by no of downstream ports followed by for DP the USB Type, width, frequency, \n\t Upstream B/W demand, Downstream B/W demand (example below)\n hub_config.in Format as follows ...\n");
  printf("2, 2, 20 // Line 1: Means 2 lanes in, 2 lanes out at 20G for UP\n");
  printf("4 // Line 2: No of downstream Ports in Hub is 4 - this means 4 more lines as follows\n");
  printf("3 1 1 5 2.0 0.4: Downstream Port 0 is USB 3; x1 in, x1 out, ; 5 GT/s, \n\tUpstream B/W demand 2.0 Gb/s, and Downstream B/W demand of 0.4 Gb/s\n");
  printf("2 0 0 0 0.5 0.1: Downstream Port 1 is USB 2; next two numbers are dont care, \n\t Upstream utilization of 0.5, and DOwnstream Utilization of 0.1\n");
  printf("3 1 3 10 10.0 18.0: Downstream Port 2 is USB 3.2; x1 wide up, x3 down; 10 GT/s, \n\t Upstream B/W demand is 10 Gb/s, and Downstream B/W demand is 18.0 Gb/s\n");
  printf("4 2 2 20 20.0 36.0: Downstream Port 3 is USB 4; 2 wide; 20 GT/s, \n\t Upstream B/W demand is 20.0 Gb/s, and Downstream B/W demand is 36.0 Gb/s\n");
  printf("-----------------------------------\n");
  //
  // Read from input file and print out
  //
  fp1=fopen("hub_config.in","r");
  fscanf(fp1, "%d %d %d", &up_in_width, &up_out_width,&up_freq);
  if(up_freq == 5) up_freq =4;
  up_in_bw=(double)up_freq * (double)up_in_width;
  up_out_bw=(double)up_freq * (double)up_out_width;
  fscanf(fp1,"%d",&num_dp);
  printf("USB Hub Specification considered:\n");
  printf("UP: x%d in, x%d out, %dGT/s, %d Downstream Ports\n",up_in_width,up_out_width,up_freq,num_dp);
  dp_in_raw_bw_aggr = dp_out_raw_bw_aggr = 0;
  for(i=0;i<num_dp;i++){
    fscanf(fp1,"%d %d %d %d %lf %lf",&dp_usb[i],&dp_in_width[i],&dp_out_width[i],&dp_freq[i],&dp_up_bw_demand[i],&dp_dn_bw_demand[i]);
    if(dp_usb[i]<3){
      dp_in_width[i]=0;
      dp_out_width[i]=0;
      dp_freq[i]=0;
    }
    if(dp_freq[i]==5) dp_freq[i]=4;
    dp_in_raw_bw_aggr += (double)(dp_in_width[i]*dp_freq[i]);
    dp_out_raw_bw_aggr += (double)(dp_out_width[i]*dp_freq[i]);
    printf("DP %d: USB %d, in width: %d, out width: %d, %d GT/s, Util (%lf up, %lf dn)\n",i,dp_usb[i],dp_in_width[i],dp_out_width[i],dp_freq[i],dp_up_bw_demand[i],dp_dn_bw_demand[i]);
    net_up_demand+= dp_up_bw_demand[i];
    net_dn_demand+= dp_dn_bw_demand[i];
    //printf("i=%d, up = %lf, dn = %lf\n",i,net_up_demand,net_dn_demand);
  }
  fclose(fp1);
  printf("Net UP B/W (%lf up, %lf dn) vs net demand by DP ports (%lf up, %lf dn) vs raw (max) b/w demand in DP %lf in %lf out\n",up_in_bw,up_out_bw,net_up_demand,net_dn_demand,dp_in_raw_bw_aggr,dp_out_raw_bw_aggr);
 
  printf("----Raw B/W Allocation to DP based on width/ frequency -------\n");
  for(i=0;i<num_dp;i++){
    raw_bw_allot_in[i]=(double)(dp_in_width[i]*dp_freq[i])/dp_in_raw_bw_aggr;
    raw_bw_allot_out[i]=(double)(dp_out_width[i]*dp_freq[i])/dp_out_raw_bw_aggr;
    printf("DP Port %d: Equitable Util: in: %lf, out: %lf\n",i,raw_bw_allot_in[i],raw_bw_allot_out[i]);
    raw_bw_allot_tot_in+=raw_bw_allot_in[i];
    raw_bw_allot_tot_out+=raw_bw_allot_out[i];
  }
  printf("Total Raw B/W allot fraction: %lf in, %lf out\n",raw_bw_allot_tot_in,raw_bw_allot_tot_out);
  printf("---------------------\n");
 
  //
  // Now the actual b/w allocation to each DP in each direction
  // based on the actual b/w demand and the fair b/w allocation
  // (lower of the two). This happens in two phases. In the first
  // phase we do the lower of the two and then any remaining b/w
  // from the available UP b/w is redistributed proportionately among
  // those DP ports whose b/w demand is more than the fair allocation
  //
  //
  printf("-----First Pass B/W allocation to DP ports-----\n");
  for(i=0;i<num_dp;i++){
    if((raw_bw_allot_in[i]*up_in_bw) < dp_up_bw_demand[i]){
      dp_actual_bw_allot_up[i]=0;
      dp_bw_demand_met_up[i]=0;
      raw_bw_allot_remain_after_first_pass_up += raw_bw_allot_in[i];
    }
    else{
      dp_actual_bw_allot_up[i]=dp_up_bw_demand[i];
      dp_bw_demand_met_up[i]=1;
    }
    //
    if((raw_bw_allot_out[i]*up_out_bw) < dp_dn_bw_demand[i]){
      dp_actual_bw_allot_dn[i]=0;
      dp_bw_demand_met_dn[i]=0;
      raw_bw_allot_remain_after_first_pass_dn += raw_bw_allot_out[i];
    }
    else{
      dp_actual_bw_allot_dn[i]=dp_dn_bw_demand[i];
      dp_bw_demand_met_dn[i]=1;
    }
    aggr_bw_allotted_up+=dp_actual_bw_allot_up[i];
    aggr_bw_allotted_dn+=dp_actual_bw_allot_dn[i];
    //
    printf("DP %d: UP B/W: %lf (demand met=%d), DN B/W: %lf (demand met = %d)\n",i,dp_actual_bw_allot_up[i],dp_bw_demand_met_up[i],dp_actual_bw_allot_dn[i],dp_bw_demand_met_dn[i]);
  }
  bw_remain_up_after_first_pass = up_in_bw-aggr_bw_allotted_up;
  bw_remain_dn_after_first_pass = up_out_bw -aggr_bw_allotted_dn;
  printf("B/W allotted after first pass: %lf up, %lf dn\n",aggr_bw_allotted_up,aggr_bw_allotted_dn);
  printf("B/W remaining to allot in second pass: %lf up, %lf dn\n",bw_remain_up_after_first_pass,bw_remain_dn_after_first_pass);
  printf("Raw B/W fractions remaining (%lf up, %lf dn)\n",raw_bw_allot_remain_after_first_pass_up,raw_bw_allot_remain_after_first_pass_dn);
  //
  //
  printf("-----Final Pass B/W allocation to DP ports-----\n");
  for(i=0;i<num_dp;i++){
    if(dp_bw_demand_met_up[i] == 0){
      dp_actual_bw_allot_up[i]=raw_bw_allot_in[i]*bw_remain_up_after_first_pass/raw_bw_allot_remain_after_first_pass_up;
      aggr_bw_allotted_up+=dp_actual_bw_allot_up[i];
    }
    if(dp_bw_demand_met_dn[i] == 0){
      dp_actual_bw_allot_dn[i]=raw_bw_allot_out[i]*bw_remain_dn_after_first_pass/raw_bw_allot_remain_after_first_pass_dn;
      aggr_bw_allotted_dn+=dp_actual_bw_allot_dn[i];
    }
 
    if((dp_in_width[i]*dp_freq[i]) > 0){
      dp_util_up[i]=dp_actual_bw_allot_up[i]/(double)(dp_in_width[i]*dp_freq[i]);
    }
    else{ // This is the USB1 and USB2 simplification
      dp_util_up[i]=1;
    }
 
    if((dp_out_width[i]*dp_freq[i]) > 0){
      dp_util_dn[i]=dp_actual_bw_allot_dn[i]/(double)(dp_out_width[i]*dp_freq[i]);
    }
    else{ // This is the USB1 and USB2 simplification
      dp_util_dn[i]=1;
    }
    if(dp_util_up[i]>dp_util_dn[i]) dp_util_link[i]=dp_util_up[i];
    else dp_util_link[i]=dp_util_dn[i];
    if(USB3_ASYM_L1_SUPPORT == 0){
      dp_util_up[i]=dp_util_dn[i]=dp_util_link[i];
    }
 
    printf("DP Port: %d, UP B/W: %lf (util: %lf), DN B/W: %lf (util: %lf), Link Util: %lf\n",i,dp_actual_bw_allot_up[i],dp_util_up[i],dp_actual_bw_allot_dn[i], dp_util_dn[i],dp_util_link[i]);
  }
  up_util_up = aggr_bw_allotted_up/up_in_bw;
  up_util_dn = aggr_bw_allotted_dn/up_out_bw;
  if(up_util_up > up_util_dn) up_util_link=up_util_up;
  else up_util_link=up_util_dn;
  if(USB3_ASYM_L1_SUPPORT == 0){
    printf("Symmetric L1\n");
    up_util_up=up_util_dn=up_util_link;
  }
  printf("Aggr B/w Allotted: (%lf up, %lf dn)\n",aggr_bw_allotted_up,aggr_bw_allotted_dn);
  printf("Hub UP utilization: up: %lf, dn: %lf, link: %lf\n",up_util_up,up_util_dn,up_util_link);
  //
 
  printf("----------------------------------------------\n");
  printf("Power in U0: %lf W",USB3_POWER_U0);
  printf("Power Savings States: Asymmetric Ux Enabled (0: N, 1:Y): %d\n",USB3_ASYM_L1_SUPPORT);
  printf("U1: Entry : %lf us, Exit: %lf us, Power: %lf W\n",USB3_ENTRY_U1,USB3_EXIT_U1,USB3_POWER_U1);
  printf("U2: Entry : %lf us, Exit: %lf us, Power: %lf W\n",USB3_ENTRY_U2,USB3_EXIT_U2,USB3_POWER_U2); 
  printf("U3: Entry : %lf us, Exit: %lf us, Power: %lf W\n",USB3_ENTRY_U3,USB3_EXIT_U3,USB3_POWER_U3);
  printf("----------------------------------------------\n");
  for(j=0;j<10;j++){
    time_tot = (double)(j+1)*100.0;
 
    printf("-------Power Savings with %lf (us) scheduling interval ---\n",time_tot);
    total_power_savings_hub_u1 = 0;
    total_power_savings_hub_u2 = 0;
    total_power_savings_hub_u3 = 0;
    //
    time_in_u0_or_entry_exit_u1=(up_util_up*time_tot)+USB3_ENTRY_U1+USB3_EXIT_U1;
    time_out_u0_or_entry_exit_u1=(up_util_dn*time_tot)+USB3_ENTRY_U1+USB3_EXIT_U1;
    time_in_u0_or_entry_exit_u2=(up_util_up*time_tot)+USB3_ENTRY_U2+USB3_EXIT_U2;
    time_out_u0_or_entry_exit_u2=(up_util_dn*time_tot)+USB3_ENTRY_U2+USB3_EXIT_U2;
    time_in_u0_or_entry_exit_u3=(up_util_up*time_tot)+USB3_ENTRY_U3+USB3_EXIT_U3;
    time_out_u0_or_entry_exit_u3=(up_util_dn*time_tot)+USB3_ENTRY_U3+USB3_EXIT_U3;
    //
    // We do each direction separately - so the power savings is divided by 2
    //
    //U1
    //
    frac_in_u1 = 1.0-(time_in_u0_or_entry_exit_u1/time_tot);
    if(frac_in_u1<0) frac_in_u1=0;
    power_savings_port_in_ux = frac_in_u1*(USB3_POWER_U0-USB3_POWER_U1)/2.0;
    //
    frac_out_u1 = 1.0-(time_out_u0_or_entry_exit_u1/time_tot);
    if(frac_out_u1<0) frac_out_u1=0;
    power_savings_port_out_ux = frac_out_u1*(USB3_POWER_U0-USB3_POWER_U1)/2.0;
    //
    total_power_savings_hub_u1 += (power_savings_port_in_ux+power_savings_port_out_ux);
    //
    printf("Hub UFP (U1): Power Savings (W): (%lf in, %lf out, %lf Total)\n",power_savings_port_in_ux,power_savings_port_out_ux,(power_savings_port_in_ux+power_savings_port_out_ux));
    //
    //U2
    //
    frac_in_u2 = 1.0-(time_in_u0_or_entry_exit_u2/time_tot);
    if(frac_in_u2<0) frac_in_u2=0;
    power_savings_port_in_ux = frac_in_u2*(USB3_POWER_U0-USB3_POWER_U2)/2.0;
    //
    frac_out_u2 = 1.0-(time_out_u0_or_entry_exit_u2/time_tot);
    if(frac_out_u2<0) frac_out_u2=0;
    power_savings_port_out_ux = frac_out_u2*(USB3_POWER_U0-USB3_POWER_U2)/2.0;
    //
    total_power_savings_hub_u2 += (power_savings_port_in_ux+power_savings_port_out_ux);
    //
    printf("Hub UFP (U2): Power Savings (W): (%lf in, %lf out, %lf Total)\n",power_savings_port_in_ux,power_savings_port_out_ux,(power_savings_port_in_ux+power_savings_port_out_ux));
    //
    // U3
    //
    frac_in_u3 = 1.0-(time_in_u0_or_entry_exit_u3/time_tot);
    if(frac_in_u3<0) frac_in_u3=0;
    power_savings_port_in_ux = frac_in_u3*(USB3_POWER_U0-USB3_POWER_U3)/2.0;
    //
    frac_out_u3 = 1.0-(time_out_u0_or_entry_exit_u3/time_tot);
    if(frac_out_u3<0) frac_out_u3=0;
    power_savings_port_out_ux = frac_out_u3*(USB3_POWER_U0-USB3_POWER_U3)/2.0;
    //
    total_power_savings_hub_u3 += (power_savings_port_in_ux+power_savings_port_out_ux);
    //
    printf("Hub UFP (U3): Power Savings (W): (%lf in, %lf out, %lf Total)\n",power_savings_port_in_ux,power_savings_port_out_ux,(power_savings_port_in_ux+power_savings_port_out_ux));
    //
    //
    // Now the DFPs
    //
    for(i=0;i<num_dp;i++){
      // U1
      time_in_u0_or_entry_exit_u1=(dp_util_up[i]*time_tot)+USB3_ENTRY_U1+USB3_EXIT_U1;     
      frac_in_u1 = 1.0-(time_in_u0_or_entry_exit_u1/time_tot);
      if(frac_in_u1<0) frac_in_u1=0;
      power_savings_port_in_ux = frac_in_u1*(USB3_POWER_U0-USB3_POWER_U1)/2.0;
      total_power_savings_hub_u1 += power_savings_port_in_ux;
      //
      time_out_u0_or_entry_exit_u1=(dp_util_dn[i]*time_tot)+USB3_ENTRY_U1+USB3_EXIT_U1;     
      frac_out_u1 = 1.0-(time_out_u0_or_entry_exit_u1/time_tot);
      if(frac_out_u1<0) frac_out_u1=0;
      power_savings_port_out_ux = frac_out_u1*(USB3_POWER_U0-USB3_POWER_U1)/2.0;
      total_power_savings_hub_u1 += power_savings_port_out_ux;
      printf("Hub DFP:%d: U1 Power Savings (W): (%lf in, %lf out, %lf Total)\n",i,power_savings_port_in_ux,power_savings_port_out_ux,(power_savings_port_in_ux+power_savings_port_out_ux));
      //
      // U2
      time_in_u0_or_entry_exit_u2=(dp_util_up[i]*time_tot)+USB3_ENTRY_U2+USB3_EXIT_U2;     
      frac_in_u2 = 1.0-(time_in_u0_or_entry_exit_u2/time_tot);
      if(frac_in_u2<0) frac_in_u2=0;
      power_savings_port_in_ux = frac_in_u2*(USB3_POWER_U0-USB3_POWER_U2)/2.0;
      total_power_savings_hub_u2 += power_savings_port_in_ux;
      //
      time_out_u0_or_entry_exit_u2=(dp_util_dn[i]*time_tot)+USB3_ENTRY_U2+USB3_EXIT_U2;     
      frac_out_u2 = 1.0-(time_out_u0_or_entry_exit_u2/time_tot);
      if(frac_out_u2<0) frac_out_u2=0;
      power_savings_port_out_ux = frac_out_u2*(USB3_POWER_U0-USB3_POWER_U2)/2.0;
      total_power_savings_hub_u2 += power_savings_port_out_ux;
      printf("Hub DFP:%d: U2 Power Savings (W): (%lf in, %lf out, %lf Total)\n",i,power_savings_port_in_ux,power_savings_port_out_ux,(power_savings_port_in_ux+power_savings_port_out_ux));
      //
      // U3
      time_in_u0_or_entry_exit_u3=(dp_util_up[i]*time_tot)+USB3_ENTRY_U3+USB3_EXIT_U3;     
      frac_in_u3 = 1.0-(time_in_u0_or_entry_exit_u3/time_tot);
      if(frac_in_u3<0) frac_in_u3=0;
      power_savings_port_in_ux = frac_in_u3*(USB3_POWER_U0-USB3_POWER_U3)/2.0;
      total_power_savings_hub_u3 += power_savings_port_in_ux;
      //
      time_out_u0_or_entry_exit_u3=(dp_util_dn[i]*time_tot)+USB3_ENTRY_U3+USB3_EXIT_U3;     
      frac_out_u3 = 1.0-(time_out_u0_or_entry_exit_u3/time_tot);
      if(frac_out_u3<0) frac_out_u3=0;
      power_savings_port_out_ux = frac_out_u3*(USB3_POWER_U0-USB3_POWER_U3)/2.0;
      total_power_savings_hub_u3 += power_savings_port_out_ux;
      printf("Hub DFP:%d: U3 Power Savings (W): (%lf in, %lf out, %lf Total)\n",i,power_savings_port_in_ux,power_savings_port_out_ux,(power_savings_port_in_ux+power_savings_port_out_ux));
 
    }
    printf("Total HUB power savings w/ U1: %lf\n",total_power_savings_hub_u1);
    printf("Total HUB power savings w/ U2: %lf\n",total_power_savings_hub_u2);
    printf("Total HUB power savings w/ U3: %lf\n",total_power_savings_hub_u3);
   
  }
 
  printf("-------------\n");
 
}
