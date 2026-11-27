============================================================
Bounties Expansion - 扩展赏金 / Extended Bounty Hunting
版本 version : v1.1.1  (2026-11-26)
适用平台     : RDR2 故事模式 / Story Mode only
============================================================

【中文】

一、这个 mod 做什么
在城镇告示板接取悬赏，追踪并押送通缉犯，领取赏金。
支持活捉与击毙两种结算，路上会有匪帮前来阻拦或劫囚。

二、安装
1. 前置：ScriptHookRDR2（V2 版）、ASI Loader（dinput8.dll）。
2. BountiesExpansion.asi 放到 RDR2.exe 同级目录。
3. BountiesExpansion 文件夹放到同一目录（内含 BountiesExpansion.ini、
   Lang.dat、Lang.en.dat）。
4. 海报头像贴图 be_portraits_nya.ytd 放入 lml\stream\（需要 LML）。
5. 进入故事模式，走到任意城镇的告示板旁。

三、按键
R     查看 / 取走海报（游戏内提示键）
Enter 确认选择
Esc   返回 / 放弃
F7    随时打开案卷（任务进行中查看通缉令）

四、常见设置（BountiesExpansion.ini）
[General]
  Debug=0            0 = 不写日志（默认）；1 = 写 BountiesExpansion.log
[Poster]
  Rivals=1           是否在接单后遭遇阻拦
  RivalsChance=100   遭遇概率（0-100）
  CustodyRivals=1    是否在押送途中遭遇劫囚
  CustodyChance=100  劫囚概率（0-100）
  HuntersOnHorse=1   1 = 骑马来袭；0 = 步行
  HuntersDismountIfStuck=0  1 = 追不上时强制下马
  HunterModel=...    拦截者使用的 ped 模型
[UI]
  ReviewKey=118      案卷热键（VK 码，118 = F7）

五、交付规则
· 活捉：把人扛进牢房，在牢房内 2 米内放下。
· 击毙：把尸体送到该镇警察局后院，4 米内卸下尸体。
· 以上两种都需要下马，不能骑在马上交差。

六、兼容性说明
· 加载“执法者徽章”类 mod 后，原版赏金猎人模型（G_M_M_BountyHunters_01）
  会无法进入战斗（只喊话不开枪）。本 mod 默认使用可正常战斗的帮派模型，
  因此两种环境下都能交战，无需修改任何其他 mod。
· 与常见马匹 / 天气类 mod 无冲突。

七、反馈问题
把 [General] 的 Debug 改为 1，重现问题后把 BountiesExpansion.log 附上。

============================================================

【English】

1. What it does
Take bounties from town boards, track the wanted man down and bring him in.
Dead or alive payouts are supported, and gangs may try to stop you on the
way out or spring the prisoner on the way back.

2. Installation
- Requires ScriptHookRDR2 (V2) and an ASI loader (dinput8.dll).
- Put BountiesExpansion.asi next to RDR2.exe.
- Put the BountiesExpansion folder next to it (BountiesExpansion.ini,
  Lang.dat, Lang.en.dat).
- Put be_portraits_nya.ytd into lml\stream\ (requires LML).
- Start Story Mode and walk up to a town bounty board.

3. Controls
R      Inspect / take the poster
Enter  Confirm
Esc    Back / decline
F7     Open the case file at any time (review an active bounty)

4. Configuration (BountiesExpansion.ini)
[General] Debug=0       0 = no log file (default), 1 = write log
[Poster]  Rivals / RivalsChance          ambush chance after taking a poster
          CustodyRivals / CustodyChance  rescue attempt on the way back
          HuntersOnHorse=1               1 = mounted, 0 = on foot
          HuntersDismountIfStuck=0       1 = dismount when they fall behind
          HunterModel=...                ped model used by the attackers
[UI]      ReviewKey=118                 case file hotkey (VK code, 118 = F7)

5. Hand-over
- Alive: carry the prisoner into the cell and set him down within 2 m.
- Dead: drop the body in the station's backyard, within 4 m.
- Both require you to be on foot; you cannot hand in from the saddle.

6. Compatibility
- With a "lawman badge" style mod loaded, the stock bounty hunter model
  (G_M_M_BountyHunters_01) cannot enter combat (it only shouts). This mod
  ships with a gang model that fights normally, so it works in both cases
  without editing any third-party files.
- No known conflicts with common horse / weather mods.

7. Reporting issues
Set [General] Debug=1, reproduce the problem, and attach
BountiesExpansion.log.

============================================================
文件清单 / Files
  BountiesExpansion.asi
  BountiesExpansion\BountiesExpansion.ini
  BountiesExpansion\Lang.dat
  BountiesExpansion\Lang.en.dat
  lml\stream\be_portraits_nya.ytd
============================================================
