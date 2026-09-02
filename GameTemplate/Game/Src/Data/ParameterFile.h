#pragma once
#include <string>
#include <vector>
#include <memory>

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @file   ParameterFile.h
		 * @brief  ステータス等をJSONファイルから読み込むための共通クラス。
		 *         ファイルが見つからない・壊れている・項目が足りない場合でも
		 *         必ず既定値を返し、ゲームが止まらないようにするのが役目。
		 *         JSONには // 形式のコメントを書いてよい(読み込み時に無視する)。
		 * @author Izumida Kiryu
		 * @date   2026/09/02
		 */
		class ParameterFile
		{
		public:
			/* コンストラクタとデストラクタ。*/
			ParameterFile();
			virtual ~ParameterFile();


		public:
			/**
			 * @brief JSONファイルを読み込む。
			 * @param pFilePath ファイルパス(実行フォルダからの相対)。
			 * @return 読み込めたらtrue。ファイルが無い・書式が壊れている場合はfalse。
			 */
			bool Load(const char* pFilePath);

			/**
			 * @brief 読み込みに成功しているか。
			 * @return 成功していればtrue。
			 */
			inline bool IsLoaded() const
			{
				return bIsLoaded_;
			}

			/**
			 * @brief 読み込んだ内容の根(ルート)を指す位置へ戻す。
			 *        取り出す位置は Enter() で潜り、Leave() で1つ戻る。
			 */
			void Reset();

			/**
			 * @brief 現在位置の子オブジェクトへ潜る。
			 * @param pKey 子オブジェクトのキー。
			 * @return 潜れたらtrue。無ければfalse(位置は動かない)。
			 */
			bool Enter(const char* pKey);

			/**
			 * @brief 現在位置の配列から、指定した番目の要素(オブジェクト)へ潜る。
			 * @param pKey   配列のキー。
			 * @param iIndex 何番目の要素か(0から)。
			 * @return 潜れたらtrue。無ければfalse(位置は動かない)。
			 */
			bool EnterArrayElement(const char* pKey, int iIndex);

			/**
			 * @brief 潜る前の位置へ1つ戻る。根にいるときは何もしない。
			 */
			void Leave();

			/**
			 * @brief 現在位置にある配列の要素数を取得する。
			 * @param pKey 配列のキー。
			 * @return 要素数。配列で無ければ0。
			 */
			int GetArraySize(const char* pKey) const;


		/* 値の取り出し。キーが無い・型が違う場合は既定値を返す。*/
		public:
			/**
			 * @brief 小数を取り出す。
			 * @param pKey     キー。
			 * @param fDefault 取り出せなかったときに返す既定値。
			 * @return 取り出した値。
			 */
			float GetFloat(const char* pKey, float fDefault) const;

			/**
			 * @brief 整数を取り出す。
			 * @param pKey     キー。
			 * @param iDefault 取り出せなかったときに返す既定値。
			 * @return 取り出した値。
			 */
			int GetInt(const char* pKey, int iDefault) const;

			/**
			 * @brief 真偽値を取り出す。
			 * @param pKey     キー。
			 * @param bDefault 取り出せなかったときに返す既定値。
			 * @return 取り出した値。
			 */
			bool GetBool(const char* pKey, bool bDefault) const;

			/**
			 * @brief 文字列を取り出す。
			 * @param pKey     キー。
			 * @param pDefault 取り出せなかったときに返す既定値。
			 * @return 取り出した値。
			 */
			std::string GetString(const char* pKey, const char* pDefault) const;

			/**
			 * @brief 小数のペアが並んだ配列を取り出す(リコイルパターンのような [a, b] の並び)。
			 * @param pKey        キー。
			 * @param vecOutFirst 1つめの値を受け取る配列。
			 * @param vecOutSecond 2つめの値を受け取る配列。
			 * @return 1組でも取り出せたらtrue。取り出せなければfalse(配列は空になる)。
			 */
			bool GetFloatPairArray(const char* pKey, std::vector<float>& vecOutFirst, std::vector<float>& vecOutSecond) const;


		private:
			/* 実装(nlohmann::json)をヘッダから隠すための内部データ。*/
			struct Impl;

			std::unique_ptr<Impl> pImpl_;	//! 読み込んだJSONと、潜っている位置の履歴。
			bool bIsLoaded_ = false;		//! 読み込みに成功したか。
		};
	}
}
