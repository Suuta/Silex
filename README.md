# Silex

レンダリングの学習用に使用しているレンダラーです。<br>



## 動作確認環境

* Windows 10 / 11
* Windows SDK:  ~ 10.0.22000
* C++ 20



## ビルド

```bat
git clone --recursive https://github.com/Suuta/Silex.git
```

プロジェクト生成ツールに [Premake](https://premake.github.io/) を使用していますが、別途インストールは必要ありません。<br>
クローン後に ***GenerateProject.bat*** を実行して *VisualStudio* ソリューションを生成します。<br>
生成後、ソリューションを開いてビルドをするか ***BuildProject.bat*** の実行でビルドが行われます。<br>
