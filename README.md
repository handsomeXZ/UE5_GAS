## Ability-示例：交互功能
1. 准备工作
    1. 碰撞描述文件（Collision.Preset)
        >InteractTrace **|** 纯查询（无物理碰撞）**|**  Ability
        >（用于标记修改可以交互物体的碰撞预设）
    2. GameplayTag标签
        >Ability.Interaction
    3. 在C++创建 **GSInteractabel** 接口 **|** **GA_InteractActive** **|** **GA_InteractPassive** **|** **GAST_WaitInteractableTarget**
        >1. **GAST_WaitInteractableTarget**（`GameplayAbilitytask`）用于启用计时的射线，对交互物体进行检测
        >2. **GA_InteractPassive**（`GameplayAbility`[**被动Ability**](https://github.com/BillEliot/GASDocumentation_Chinese#4641-%E8%A2%AB%E5%8A%A8ability)）用于调用**GAST_WaitInteractableTarget**的**Wait for Interactable Target**、在 HUD 上显示交互提示、发送EventData
        >3. **GA_InteractActive**（`GameplayAbility`-主动Ability）
        >4. **GSInteractabel 接口**用于提供各种交互通用的功能：
        >* `GetInteractionDuration`：设置该交互功能是否需要有触发时间
        >* `PreInteract`、`PostInteract` ：在交互物体内继承接口并重写**PreInteract**和**PostInteract**来实现不同的交互功能。PreInteract会在**Duration**不为0时生效
        >* `GetInteractSyncType`：设置该交互的同步方式：
        >>* *OnlyServerWait* - 客户端预测性地调用 PreInteract()。 
        >>* *OnlyClientWait* - 客户端等待服务器调用 PreInteract()。 如果我们正在激活另一个 ASC（玩家）上的能力并希望将动作或动画与我们的交互持续时间计时器同步，这将非常有用。 
        >>* *BothWait* - 客户端和服务器在调用 PreInteract() 之前相互等待。


2. GAST_WaitInteractableTarget::ClipCameraRayToAbilityRange 
帮助理解：
![ClipCameraRayToAbilityRange.png](https://raw.githubusercontent.com/handsomeXZ/UE5_GAS/main/Source/GASTest/Private/GAS/ClipCameraRayToAbilityRange.png)



tips：
1. UAbilityTask的构造函数中强制硬编码允许最多1000个同时运行的AbilityTask, 当设计那些同时拥有数百个Character的游戏(像RTS)的GameplayAbility时要注意这一点.


更新：
1. 待更新示例文件。。。
2. 已经更新GSInteractabel，Ability部分内容
3. 已经更新GSInteractabel，Ability部分内容，GAST_WaitInteractableTarget