# XAMIDI
【DirectX】基于XAudio2的midi合成器——XAMIDI<br/>
原帖来源：【DirectX】基于XAudio2的midi合成器——XAMIDI https://www.0xaa55.com/thread-26601-1-1.html （出处：技术宅的结界，转载请保留出处。）<br/>
参考来源：【C语言】Midi文件播放器（可跨平台）https://www.0xaa55.com/thread-1489-1-1.html （出处：技术宅的结界）<br/>
GitHub：https://github.com/yym1024/XAMIDI <br/>
<br/>
XAMIDI 是一个基于 XAudio2 的 midi1.0 合成器，当前的版本为0.0（试验版本），等将来各位网友及大神提出相关的意见后继续完善再发布1.x版本（正式版本），目前打算再等 midi2.0 标准完善后再升级到2.x版本。<br/>
XAMIDI 的设计初衷是因为 XAudio2 取代了 DSound，而 Microsoft 官方未提供取代 DMusic 的音乐组件，DMusic 只提供了输出到 DSound 的交互接口，未提供输出到 XAudio2 或其它音频 API 的交互接口。<br/>
XAMIDI 相比 winmm.dll 的 midiOut 来说，拥有更快的启动速度、更低的延迟、更多的复音数、更加丰富的音色库等优势，而且 midiOut 只有16个通道还只能创建一个实例，而 XAMIDI 不仅可以创建多个实例、每个实例都有1000个独立的通道组（每组16个通过，共16000个通道），还可以给它设置特效以及3D音效等功能。<br/>
