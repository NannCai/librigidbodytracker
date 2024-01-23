\ Model RAP
\ LP format - for model browsing. Use MPS format to capture full model detail.
Minimize
  6 assign[a0,t7] + 50 assign[a0,t6_t1] + 98 assign[a0,t9_t1]
   + 53 assign[a0,t8_t1] + 54 assign[a1,t4_t1] + 81 assign[a1,t0_t3]
   + 100 assign[a2,t5_t3] + 71 assign[a2,t7_t1] + 4 assign[a3,t8_t4]
   + 35 assign[a3,t3_t1] + 20 assign[a3,t9_t1] + 17 assign[a3,t0_t8]
   + 78 assign[a3,t5_t2] + 53 assign[a4,t3_t6] + 64 assign[a4,t8_t1]
   + 66 assign[a4,t9_t6] + 17 assign[a5,t9_t2] + 10 assign[a6,t0_t8]
   + 4 assign[a6,t2_t1] + 87 assign[a6,t4] + 21 assign[a6,t8_t6]
   + 10000 assign[a4,t50] + 10000 assign[a5,t51] + 10000 assign[a3,t52]
   + 10000 assign[a6,t53] + 10000 assign[a1,t54] + 10000 assign[a2,t55]
   + 10000 assign[a0,t56]
Subject To
 groupsConstrs[t54]: assign[a1,t54] <= 1
 groupsConstrs[t5_t3]: assign[a2,t5_t3] <= 1
 groupsConstrs[t5_t2]: assign[a3,t5_t2] <= 1
 groupsConstrs[t6_t1]: assign[a0,t6_t1] <= 1
 groupsConstrs[t0_t8]: assign[a3,t0_t8] + assign[a6,t0_t8] <= 1
 groupsConstrs[t0_t3]: assign[a1,t0_t3] <= 1
 groupsConstrs[t51]: assign[a5,t51] <= 1
 groupsConstrs[t9_t2]: assign[a5,t9_t2] <= 1
 groupsConstrs[t8_t4]: assign[a3,t8_t4] <= 1
 groupsConstrs[t4_t1]: assign[a1,t4_t1] <= 1
 groupsConstrs[t9_t1]: assign[a0,t9_t1] + assign[a3,t9_t1] <= 1
 groupsConstrs[t55]: assign[a2,t55] <= 1
 groupsConstrs[t53]: assign[a6,t53] <= 1
 groupsConstrs[t3_t1]: assign[a3,t3_t1] <= 1
 groupsConstrs[t52]: assign[a3,t52] <= 1
 groupsConstrs[t4]: assign[a6,t4] <= 1
 groupsConstrs[t56]: assign[a0,t56] <= 1
 groupsConstrs[t50]: assign[a4,t50] <= 1
 groupsConstrs[t7]: assign[a0,t7] <= 1
 groupsConstrs[t8_t1]: assign[a0,t8_t1] + assign[a4,t8_t1] <= 1
 groupsConstrs[t9_t6]: assign[a4,t9_t6] <= 1
 groupsConstrs[t8_t6]: assign[a6,t8_t6] <= 1
 groupsConstrs[t2_t1]: assign[a6,t2_t1] <= 1
 groupsConstrs[t7_t1]: assign[a2,t7_t1] <= 1
 groupsConstrs[t3_t6]: assign[a4,t3_t6] <= 1
 agentsConstrs[a4]: assign[a4,t3_t6] + assign[a4,t8_t1] + assign[a4,t9_t6]
   + assign[a4,t50] = 1
 agentsConstrs[a5]: assign[a5,t9_t2] + assign[a5,t51] = 1
 agentsConstrs[a3]: assign[a3,t8_t4] + assign[a3,t3_t1] + assign[a3,t9_t1]
   + assign[a3,t0_t8] + assign[a3,t5_t2] + assign[a3,t52] = 1
 agentsConstrs[a6]: assign[a6,t0_t8] + assign[a6,t2_t1] + assign[a6,t4]
   + assign[a6,t8_t6] + assign[a6,t53] = 1
 agentsConstrs[a1]: assign[a1,t4_t1] + assign[a1,t0_t3] + assign[a1,t54]
   = 1
 agentsConstrs[a2]: assign[a2,t5_t3] + assign[a2,t7_t1] + assign[a2,t55]
   = 1
 agentsConstrs[a0]: assign[a0,t7] + assign[a0,t6_t1] + assign[a0,t9_t1]
   + assign[a0,t8_t1] + assign[a0,t56] = 1
 t5Constr: assign[a2,t5_t3] + assign[a3,t5_t2] <= 1
 t9Constr: assign[a0,t9_t1] + assign[a3,t9_t1] + assign[a4,t9_t6]
   + assign[a5,t9_t2] <= 1
 t6Constr: assign[a0,t6_t1] + assign[a4,t3_t6] + assign[a4,t9_t6]
   + assign[a6,t8_t6] <= 1
 t2Constr: assign[a3,t5_t2] + assign[a5,t9_t2] + assign[a6,t2_t1] <= 1
 t7Constr: assign[a0,t7] + assign[a2,t7_t1] <= 1
 t0Constr: assign[a1,t0_t3] + assign[a3,t0_t8] + assign[a6,t0_t8] <= 1
 t8Constr: assign[a0,t8_t1] + assign[a3,t8_t4] + assign[a3,t0_t8]
   + assign[a4,t8_t1] + assign[a6,t0_t8] + assign[a6,t8_t6] <= 1
 t4Constr: assign[a1,t4_t1] + assign[a3,t8_t4] + assign[a6,t4] <= 1
 t1Constr: assign[a0,t6_t1] + assign[a0,t9_t1] + assign[a0,t8_t1]
   + assign[a1,t4_t1] + assign[a2,t7_t1] + assign[a3,t3_t1]
   + assign[a3,t9_t1] + assign[a4,t8_t1] + assign[a6,t2_t1] <= 1
 t3Constr: assign[a1,t0_t3] + assign[a2,t5_t3] + assign[a3,t3_t1]
   + assign[a4,t3_t6] <= 1
Bounds
Binaries
 assign[a0,t7] assign[a0,t6_t1] assign[a0,t9_t1] assign[a0,t8_t1]
 assign[a1,t4_t1] assign[a1,t0_t3] assign[a2,t5_t3] assign[a2,t7_t1]
 assign[a3,t8_t4] assign[a3,t3_t1] assign[a3,t9_t1] assign[a3,t0_t8]
 assign[a3,t5_t2] assign[a4,t3_t6] assign[a4,t8_t1] assign[a4,t9_t6]
 assign[a5,t9_t2] assign[a6,t0_t8] assign[a6,t2_t1] assign[a6,t4]
 assign[a6,t8_t6] assign[a4,t50] assign[a5,t51] assign[a3,t52]
 assign[a6,t53] assign[a1,t54] assign[a2,t55] assign[a0,t56]
End
