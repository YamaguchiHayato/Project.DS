#include "stdafx.h"
#include "Src/Data/ParameterFile.h"
#include <fstream>

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @struct ParameterFile::Impl
		 * @brief  nlohmann::json をヘッダへ出さないための内部データ。
		 *         読み込んだ全体(ルート)と、いま値を取り出す位置を持つ。
		 */
		struct ParameterFile::Impl
		{
			nlohmann::json jsonRoot_;							//! 読み込んだJSON全体。
			std::vector<const nlohmann::json*> vecNodeStack_;	//! 潜っている位置の履歴(末尾がいまの位置)。

			/**
			 * @brief いま値を取り出す位置を取得する。
			 * @return いまの位置。読み込めていなければnullptr。
			 */
			const nlohmann::json* GetCurrent() const
			{
				return vecNodeStack_.empty() ? nullptr : vecNodeStack_.back();
			}
		};


		ParameterFile::ParameterFile()
			: pImpl_(std::make_unique<Impl>())
		{}


		ParameterFile::~ParameterFile() = default;


		bool ParameterFile::Load(const char* pFilePath)
		{
			bIsLoaded_ = false;
			pImpl_->vecNodeStack_.clear();

			/* ファイルを開く。無ければ既定値で動かすので、ここでは失敗を返すだけにする。*/
			std::ifstream file(pFilePath);
			if (!file.is_open())
				return false;

			/*
			 * 書式が壊れていても落とさない。
			 * 第3引数=false で例外を投げず、第4引数=true で // コメントを無視する。
			 */
			pImpl_->jsonRoot_ = nlohmann::json::parse(file, nullptr, false, true);

			/* 解析に失敗していれば既定値で動かす。*/
			if (pImpl_->jsonRoot_.is_discarded() || !pImpl_->jsonRoot_.is_object())
			{
				pImpl_->jsonRoot_ = nlohmann::json::object();
				return false;
			}

			pImpl_->vecNodeStack_.push_back(&pImpl_->jsonRoot_);
			bIsLoaded_ = true;

			return true;
		}


		void ParameterFile::Reset()
		{
			/* 読み込めていなければ位置も無い。*/
			if (!bIsLoaded_)
				return;

			pImpl_->vecNodeStack_.clear();
			pImpl_->vecNodeStack_.push_back(&pImpl_->jsonRoot_);
		}


		bool ParameterFile::Enter(const char* pKey)
		{
			const nlohmann::json* pCurrent = pImpl_->GetCurrent();

			/* 現在位置が無ければ潜れない。*/
			if (pCurrent == nullptr)
				return false;

			/* 子を探す。オブジェクトでなければ潜らない。*/
			const auto it = pCurrent->find(pKey);
			if (it == pCurrent->end() || !it->is_object())
				return false;

			pImpl_->vecNodeStack_.push_back(&(*it));

			return true;
		}


		bool ParameterFile::EnterArrayElement(const char* pKey, int iIndex)
		{
			const nlohmann::json* pCurrent = pImpl_->GetCurrent();

			/* 現在位置が無ければ潜れない。*/
			if (pCurrent == nullptr || iIndex < 0)
				return false;

			/* 配列を探す。範囲外や、要素がオブジェクトでなければ潜らない。*/
			const auto it = pCurrent->find(pKey);
			if (it == pCurrent->end() || !it->is_array() || static_cast<size_t>(iIndex) >= it->size())
				return false;

			const nlohmann::json& jsonElement = (*it)[static_cast<size_t>(iIndex)];
			if (!jsonElement.is_object())
				return false;

			pImpl_->vecNodeStack_.push_back(&jsonElement);

			return true;
		}


		void ParameterFile::Leave()
		{
			/* 根まで戻っていれば何もしない(履歴の先頭は必ず根)。*/
			if (pImpl_->vecNodeStack_.size() <= 1)
				return;

			pImpl_->vecNodeStack_.pop_back();
		}


		int ParameterFile::GetArraySize(const char* pKey) const
		{
			const nlohmann::json* pCurrent = pImpl_->GetCurrent();

			/* 現在位置が無ければ0。*/
			if (pCurrent == nullptr)
				return 0;

			const auto it = pCurrent->find(pKey);
			if (it == pCurrent->end() || !it->is_array())
				return 0;

			return static_cast<int>(it->size());
		}


		float ParameterFile::GetFloat(const char* pKey, float fDefault) const
		{
			/* 現在位置が無い、またはキーが無ければ既定値。*/
			const nlohmann::json* pCurrent = pImpl_->GetCurrent();
			if (pCurrent == nullptr)
				return fDefault;

			const auto it = pCurrent->find(pKey);
			if (it == pCurrent->end() || !it->is_number())
				return fDefault;

			return it->get<float>();
		}


		int ParameterFile::GetInt(const char* pKey, int iDefault) const
		{
			/* 現在位置が無い、またはキーが無ければ既定値。*/
			const nlohmann::json* pCurrent = pImpl_->GetCurrent();
			if (pCurrent == nullptr)
				return iDefault;

			const auto it = pCurrent->find(pKey);
			if (it == pCurrent->end() || !it->is_number())
				return iDefault;

			/* 8 と書いても 8.0 と書いても整数として受け取れるようにする。*/
			if (it->is_number_integer())
				return it->get<int>();

			return static_cast<int>(it->get<double>());
		}


		bool ParameterFile::GetBool(const char* pKey, bool bDefault) const
		{
			/* 現在位置が無い、またはキーが無ければ既定値。*/
			const nlohmann::json* pCurrent = pImpl_->GetCurrent();
			if (pCurrent == nullptr)
				return bDefault;

			const auto it = pCurrent->find(pKey);
			if (it == pCurrent->end() || !it->is_boolean())
				return bDefault;

			return it->get<bool>();
		}


		std::string ParameterFile::GetString(const char* pKey, const char* pDefault) const
		{
			/* 現在位置が無い、またはキーが無ければ既定値。*/
			const nlohmann::json* pCurrent = pImpl_->GetCurrent();
			if (pCurrent == nullptr)
				return pDefault;

			const auto it = pCurrent->find(pKey);
			if (it == pCurrent->end() || !it->is_string())
				return pDefault;

			return it->get<std::string>();
		}


		bool ParameterFile::GetFloatPairArray(const char* pKey, std::vector<float>& vecOutFirst, std::vector<float>& vecOutSecond) const
		{
			vecOutFirst.clear();
			vecOutSecond.clear();

			/* 現在位置が無い、またはキーが無ければ空のまま返す。*/
			const nlohmann::json* pCurrent = pImpl_->GetCurrent();
			if (pCurrent == nullptr)
				return false;

			const auto it = pCurrent->find(pKey);
			if (it == pCurrent->end() || !it->is_array())
				return false;

			/* [a, b] の形になっている要素だけを拾う。*/
			for (const auto& element : *it)
			{
				if (!element.is_array() || element.size() < 2)
					continue;

				if (!element[0].is_number() || !element[1].is_number())
					continue;

				vecOutFirst.push_back(element[0].get<float>());
				vecOutSecond.push_back(element[1].get<float>());
			}

			return !vecOutFirst.empty();
		}
	}
}
