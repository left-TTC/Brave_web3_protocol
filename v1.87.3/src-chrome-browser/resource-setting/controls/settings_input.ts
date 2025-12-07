// Copyright 2024 The Brave Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import '//resources/cr_elements/cr_input/cr_input.js';
import '//resources/cr_elements/cr_shared_vars.css.js';
import '/shared/settings/controls/cr_policy_pref_indicator.js';

import {CrInputElement} from '//resources/cr_elements/cr_input/cr_input.js';
import {PolymerElement} from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {CrPolicyPrefMixin} from '/shared/settings/controls/cr_policy_pref_mixin.js';
import {PrefControlMixin} from '/shared/settings/controls/pref_control_mixin.js';

import {getTemplate} from './settings_input.html.js';

export interface SettingsInputElement {
    $: {
        input: CrInputElement,
    };
}

// 继承 PrefControlMixin 和 CrPolicyPrefMixin 是关键
const SettingsInputElementBase =
    CrPolicyPrefMixin(PrefControlMixin(PolymerElement));

export class SettingsInputElement extends SettingsInputElementBase {
    static get is() {
        return 'settings-input';
    }

    static get template() {
        // 假设 getTemplate() 引入了 settings_input.html 的模板
        return getTemplate();
    }

    static get properties() {
        return {
            /** 组件显示的标签文字 (用于 cr-input 的 label) */
            label: String,

            /** 占位符文字 */
            placeholder: String,

            /** 是否禁用 */
            disabled: {
                type: Boolean,
                value: false,
                reflectToAttribute: true,
            },

            /**
             * 内部绑定的值，用于和 cr-input 的 value 双向绑定。
             * 这是接收 pref.value 并发送用户输入值的中间变量。
             */
            bindingValue_: {
                type: String,
                value: '',
            },
        };
    }

    static get observers() {
        return [
            // 监听 pref.value 的变化，以更新输入框
            'resetInput_(pref.value)',
        ];
    }

    declare label: string;
    declare placeholder: string;
    declare disabled: boolean;
    declare bindingValue_: string;

    /**
     * 当后端的 Pref 值发生变化（或者初始化）时，更新输入框的显示内容。
     * 这确保了输入框在加载时显示 Pref 的当前值。
     */
    private resetInput_() {
        // 1. 获取后端 Pref 的值
        const prefValue = this.pref && this.pref.value !== undefined ? 
                            this.pref.value : 
                            '';
        
        // 2. 只有当当前显示的值和 Pref 不一致时才更新
        if (this.bindingValue_ !== prefValue) {
            this.bindingValue_ = prefValue as string;
        }
    }

    /**
     * 当用户完成输入（回车或失去焦点）时触发。
     */
    private onInputChange_() {
        if (!this.pref) {
            return;
        }

        // 简单的校验：如果值没变，就不发送
        if (this.bindingValue_ !== this.pref.value) {
            // 写入 Pref。由于是 String Pref，直接设置即可。
            this.set('pref.value', this.bindingValue_);
            
            // 发送标准的 settings 事件，方便父组件监听（如果需要）
            this.dispatchEvent(new CustomEvent(
                'settings-control-change', {bubbles: true, composed: true}));
        }
    }

    /**
     * 判断输入框是否应该被禁用
     */
    private shouldDisable_(): boolean {
        return this.disabled || this.isPrefEnforced();
    }
}

declare global {
    interface HTMLElementTagNameMap {
        'settings-input': SettingsInputElement;
    }
}

customElements.define(SettingsInputElement.is, SettingsInputElement);